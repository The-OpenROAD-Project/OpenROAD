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
#line 25 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"

#include <cstdlib>
#include <string>

#include "Report.hh"
#include "PortDirection.hh"
#include "VerilogReader.hh"
#include "verilog/VerilogReaderPvt.hh"
#include "verilog/VerilogScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define loc_line(loc) loc.begin.line

void
sta::VerilogParse::error(const location_type &loc,
                         const std::string &msg)
{
  reader->report()->fileError(164,reader->filename(),loc.begin.line,
                              "%s",msg.c_str());
}

#line 68 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"


#include "VerilogParse.hh"




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

#line 55 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
namespace sta {
#line 166 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"

  /// Build a parser object.
  VerilogParse::VerilogParse (VerilogScanner *scanner_yyarg, VerilogReader *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  VerilogParse::~VerilogParse ()
  {}

  VerilogParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  VerilogParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  VerilogParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  VerilogParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}


  template <typename Base>
  VerilogParse::symbol_kind_type
  VerilogParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  VerilogParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  VerilogParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  VerilogParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  VerilogParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  VerilogParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  VerilogParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  VerilogParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  VerilogParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  VerilogParse::symbol_kind_type
  VerilogParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  VerilogParse::symbol_kind_type
  VerilogParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  VerilogParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  VerilogParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  VerilogParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  VerilogParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  VerilogParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  VerilogParse::symbol_kind_type
  VerilogParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  VerilogParse::stack_symbol_type::stack_symbol_type ()
  {}

  VerilogParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  VerilogParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  VerilogParse::stack_symbol_type&
  VerilogParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  VerilogParse::stack_symbol_type&
  VerilogParse::stack_symbol_type::operator= (stack_symbol_type& that)
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
  VerilogParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    switch (yysym.kind ())
    {
      case symbol_kind::S_CONSTANT: // CONSTANT
#line 120 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                    { delete (yysym.value.string); }
#line 379 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
        break;

      case symbol_kind::S_STRING: // STRING
#line 119 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                    { delete (yysym.value.string); }
#line 385 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
        break;

      case symbol_kind::S_attr_spec_value: // attr_spec_value
#line 121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                    { delete (yysym.value.string); }
#line 391 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
        break;

      default:
        break;
    }
  }

#if YYDEBUG
  template <typename Base>
  void
  VerilogParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
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
  VerilogParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  VerilogParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  VerilogParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  VerilogParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  VerilogParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  VerilogParse::debug_level_type
  VerilogParse::debug_level () const
  {
    return yydebug_;
  }

  void
  VerilogParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  VerilogParse::state_type
  VerilogParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  VerilogParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  VerilogParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  VerilogParse::operator() ()
  {
    return parse ();
  }

  int
  VerilogParse::parse ()
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
  case 5: // module: attr_instance_seq MODULE ID ';' stmts ENDMODULE
#line 138 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { reader->makeModule((yystack_[3].value.string), new sta::VerilogNetSeq,(yystack_[1].value.stmt_seq), (yystack_[5].value.attr_stmt_seq), loc_line(yystack_[4].location));}
#line 662 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 6: // module: attr_instance_seq MODULE ID '(' ')' ';' stmts ENDMODULE
#line 140 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { reader->makeModule((yystack_[5].value.string), new sta::VerilogNetSeq,(yystack_[1].value.stmt_seq), (yystack_[7].value.attr_stmt_seq), loc_line(yystack_[6].location));}
#line 668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 7: // module: attr_instance_seq MODULE ID '(' port_list ')' ';' stmts ENDMODULE
#line 142 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { reader->makeModule((yystack_[6].value.string), (yystack_[4].value.nets), (yystack_[1].value.stmt_seq), (yystack_[8].value.attr_stmt_seq), loc_line(yystack_[7].location)); }
#line 674 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 8: // module: attr_instance_seq MODULE ID '(' port_dcls ')' ';' stmts ENDMODULE
#line 144 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { reader->makeModule((yystack_[6].value.string), (yystack_[4].value.stmt_seq), (yystack_[1].value.stmt_seq), (yystack_[8].value.attr_stmt_seq), loc_line(yystack_[7].location)); }
#line 680 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 9: // port_list: port
#line 149 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = new sta::VerilogNetSeq;
	  (yylhs.value.nets)->push_back((yystack_[0].value.net));
	}
#line 688 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 10: // port_list: port_list ',' port
#line 153 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.nets)->push_back((yystack_[0].value.net)); }
#line 694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 11: // port: port_expr
#line 157 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 700 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 12: // port: '.' ID '(' ')'
#line 159 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefScalar((yystack_[2].value.string), nullptr);}
#line 706 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 13: // port: '.' ID '(' port_expr ')'
#line 161 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefScalar((yystack_[3].value.string), (yystack_[1].value.net));}
#line 712 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 14: // port_expr: port_ref
#line 165 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 718 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 15: // port_expr: '{' port_refs '}'
#line 167 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetConcat((yystack_[1].value.nets)); }
#line 724 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 16: // port_refs: port_ref
#line 171 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = new sta::VerilogNetSeq;
	  (yylhs.value.nets)->push_back((yystack_[0].value.net));
	}
#line 732 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 17: // port_refs: port_refs ',' port_ref
#line 175 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.nets)->push_back((yystack_[0].value.net)); }
#line 738 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 18: // port_ref: net_scalar
#line 179 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 19: // port_ref: net_bit_select
#line 180 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 750 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 20: // port_ref: net_part_select
#line 181 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 756 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 21: // port_dcls: port_dcl
#line 186 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = new sta::VerilogStmtSeq;
	  (yylhs.value.stmt_seq)->push_back((yystack_[0].value.stmt));
	}
#line 764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 22: // port_dcls: port_dcls ',' port_dcl
#line 190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = (yystack_[2].value.stmt_seq);
	  (yystack_[2].value.stmt_seq)->push_back((yystack_[0].value.stmt));
	}
#line 772 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 23: // port_dcls: port_dcls ',' dcl_arg
#line 194 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        {
	  sta::VerilogDcl *dcl = dynamic_cast<sta::VerilogDcl*>((yystack_[2].value.stmt_seq)->back());
	  dcl->appendArg((yystack_[0].value.dcl_arg));
	  (yylhs.value.stmt_seq) = (yystack_[2].value.stmt_seq);
	}
#line 782 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 24: // port_dcl: attr_instance_seq port_dcl_type dcl_arg
#line 203 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeDcl((yystack_[1].value.port_type), (yystack_[0].value.dcl_arg), (yystack_[2].value.attr_stmt_seq), loc_line(yystack_[1].location)); }
#line 788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 25: // port_dcl: attr_instance_seq port_dcl_type '[' INT ':' INT ']' dcl_arg
#line 205 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeDclBus((yystack_[6].value.port_type), (yystack_[4].value.ival), (yystack_[2].value.ival), (yystack_[0].value.dcl_arg), (yystack_[7].value.attr_stmt_seq), loc_line(yystack_[6].location)); }
#line 794 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 26: // port_dcl_type: INPUT
#line 209 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
              { (yylhs.value.port_type) = sta::PortDirection::input(); }
#line 800 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 27: // port_dcl_type: INPUT WIRE
#line 210 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                   { (yylhs.value.port_type) = sta::PortDirection::input(); }
#line 806 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 28: // port_dcl_type: INOUT
#line 211 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
              { (yylhs.value.port_type) = sta::PortDirection::bidirect(); }
#line 812 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 29: // port_dcl_type: INOUT REG
#line 212 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                  { (yylhs.value.port_type) = sta::PortDirection::bidirect(); }
#line 818 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 30: // port_dcl_type: INOUT WIRE
#line 213 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                   { (yylhs.value.port_type) = sta::PortDirection::bidirect(); }
#line 824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 31: // port_dcl_type: OUTPUT
#line 214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
               { (yylhs.value.port_type) = sta::PortDirection::output(); }
#line 830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 32: // port_dcl_type: OUTPUT WIRE
#line 215 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                    { (yylhs.value.port_type) = sta::PortDirection::output(); }
#line 836 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 33: // port_dcl_type: OUTPUT REG
#line 216 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                   { (yylhs.value.port_type) = sta::PortDirection::output(); }
#line 842 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 34: // stmts: %empty
#line 221 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = new sta::VerilogStmtSeq; }
#line 848 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 35: // stmts: stmts stmt
#line 223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { if ((yystack_[0].value.stmt)) (yystack_[1].value.stmt_seq)->push_back((yystack_[0].value.stmt)); }
#line 854 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 36: // stmts: stmts stmt_seq
#line 226 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { sta::VerilogStmtSeq::Iterator iter((yystack_[0].value.stmt_seq));
	  while (iter.hasNext())
	    (yystack_[1].value.stmt_seq)->push_back(iter.next());
	  delete (yystack_[0].value.stmt_seq);
	}
#line 864 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 37: // stmt: parameter
#line 234 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 870 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 38: // stmt: defparam
#line 235 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 876 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 39: // stmt: declaration
#line 236 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 882 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 40: // stmt: instance
#line 237 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 888 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 41: // stmt: specify_block
#line 238 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 894 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 42: // stmt: error ';'
#line 240 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { yyerrok; (yylhs.value.stmt) = nullptr; }
#line 900 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 43: // stmt_seq: continuous_assign
#line 244 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = (yystack_[0].value.stmt_seq); }
#line 906 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 44: // specify_block: SPECIFY specify_stmts ENDSPECIFY
#line 256 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 912 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 45: // specify_stmts: SPECPARAM parameter_dcl ';'
#line 261 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = nullptr; }
#line 918 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 46: // specify_stmts: specify_stmts SPECPARAM parameter_dcl ';'
#line 263 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = nullptr; }
#line 924 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 47: // parameter: PARAMETER parameter_dcls ';'
#line 269 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 930 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 48: // parameter: PARAMETER '[' INT ':' INT ']' parameter_dcls ';'
#line 271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 936 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 49: // parameter_dcls: parameter_dcl
#line 276 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 942 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 50: // parameter_dcls: parameter_dcls ',' parameter_dcl
#line 278 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 948 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 51: // parameter_dcl: ID '=' parameter_expr
#line 283 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[2].value.string); (yylhs.value.stmt) = nullptr; }
#line 954 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 52: // parameter_dcl: ID '=' STRING
#line 285 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[2].value.string); delete (yystack_[0].value.string); (yylhs.value.stmt) = nullptr; }
#line 960 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 53: // parameter_expr: ID
#line 290 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[0].value.string); (yylhs.value.ival) = 0; }
#line 966 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 54: // parameter_expr: '`' ID
#line 292 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[0].value.string); (yylhs.value.ival) = 0; }
#line 972 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 55: // parameter_expr: CONSTANT
#line 294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[0].value.string); (yylhs.value.ival) = 0; }
#line 978 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 56: // parameter_expr: INT
#line 295 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[0].value.ival); }
#line 984 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 57: // parameter_expr: '-' parameter_expr
#line 297 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = - (yystack_[0].value.ival); }
#line 990 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 58: // parameter_expr: parameter_expr '+' parameter_expr
#line 299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[2].value.ival) + (yystack_[0].value.ival); }
#line 996 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 59: // parameter_expr: parameter_expr '-' parameter_expr
#line 301 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[2].value.ival) - (yystack_[0].value.ival); }
#line 1002 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 60: // parameter_expr: parameter_expr '*' parameter_expr
#line 303 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[2].value.ival) * (yystack_[0].value.ival); }
#line 1008 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 61: // parameter_expr: parameter_expr '/' parameter_expr
#line 305 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[2].value.ival) / (yystack_[0].value.ival); }
#line 1014 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 62: // parameter_expr: '(' parameter_expr ')'
#line 307 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[1].value.ival); }
#line 1020 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 63: // defparam: DEFPARAM param_values ';'
#line 312 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 1026 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 64: // param_values: param_value
#line 317 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 1032 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 65: // param_values: param_values ',' param_value
#line 319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = nullptr; }
#line 1038 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 66: // param_value: ID '=' parameter_expr
#line 324 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[2].value.string); (yylhs.value.stmt) = nullptr; }
#line 1044 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 67: // param_value: ID '=' STRING
#line 326 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { delete (yystack_[2].value.string); delete (yystack_[0].value.string); (yylhs.value.stmt) = nullptr; }
#line 1050 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 68: // declaration: attr_instance_seq dcl_type dcl_args ';'
#line 331 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeDcl((yystack_[2].value.port_type), (yystack_[1].value.dcl_arg_seq), (yystack_[3].value.attr_stmt_seq), loc_line(yystack_[2].location)); }
#line 1056 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 69: // declaration: attr_instance_seq dcl_type '[' INT ':' INT ']' dcl_args ';'
#line 333 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeDclBus((yystack_[7].value.port_type), (yystack_[5].value.ival), (yystack_[3].value.ival), (yystack_[1].value.dcl_arg_seq), (yystack_[8].value.attr_stmt_seq),loc_line(yystack_[7].location)); }
#line 1062 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 70: // dcl_type: INPUT
#line 337 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
              { (yylhs.value.port_type) = sta::PortDirection::input(); }
#line 1068 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 71: // dcl_type: INOUT
#line 338 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
              { (yylhs.value.port_type) = sta::PortDirection::bidirect(); }
#line 1074 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 72: // dcl_type: OUTPUT
#line 339 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
               { (yylhs.value.port_type) = sta::PortDirection::output(); }
#line 1080 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 73: // dcl_type: SUPPLY0
#line 340 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                { (yylhs.value.port_type) = sta::PortDirection::ground(); }
#line 1086 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 74: // dcl_type: SUPPLY1
#line 341 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
                { (yylhs.value.port_type) = sta::PortDirection::power(); }
#line 1092 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 75: // dcl_type: TRI
#line 342 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
            { (yylhs.value.port_type) = sta::PortDirection::tristate(); }
#line 1098 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 76: // dcl_type: WAND
#line 343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
             { (yylhs.value.port_type) = sta::PortDirection::internal(); }
#line 1104 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 77: // dcl_type: WIRE
#line 344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
             { (yylhs.value.port_type) = sta::PortDirection::internal(); }
#line 1110 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 78: // dcl_type: WOR
#line 345 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
            { (yylhs.value.port_type) = sta::PortDirection::internal(); }
#line 1116 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 79: // dcl_args: dcl_arg
#line 350 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.dcl_arg_seq) = new sta::VerilogDclArgSeq;
	  (yylhs.value.dcl_arg_seq)->push_back((yystack_[0].value.dcl_arg));
	}
#line 1124 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 80: // dcl_args: dcl_args ',' dcl_arg
#line 354 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.dcl_arg_seq)->push_back((yystack_[0].value.dcl_arg));
	  (yylhs.value.dcl_arg_seq) = (yystack_[2].value.dcl_arg_seq);
	}
#line 1132 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 81: // dcl_arg: ID
#line 361 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.dcl_arg) = reader->makeDclArg((yystack_[0].value.string)); }
#line 1138 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 82: // dcl_arg: net_assignment
#line 363 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.dcl_arg) = reader->makeDclArg((yystack_[0].value.assign)); }
#line 1144 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 83: // continuous_assign: ASSIGN net_assignments ';'
#line 368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = (yystack_[1].value.stmt_seq); }
#line 1150 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 84: // net_assignments: net_assignment
#line 373 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt_seq) = new sta::VerilogStmtSeq();
	  (yylhs.value.stmt_seq)->push_back((yystack_[0].value.assign));
	}
#line 1158 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 85: // net_assignments: net_assignments ',' net_assignment
#line 377 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.stmt_seq)->push_back((yystack_[0].value.assign)); }
#line 1164 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 86: // net_assignment: net_assign_lhs '=' net_expr
#line 382 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.assign) = reader->makeAssign((yystack_[2].value.net), (yystack_[0].value.net), loc_line(yystack_[2].location)); }
#line 1170 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 87: // net_assign_lhs: net_named
#line 386 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1176 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 88: // net_assign_lhs: net_expr_concat
#line 387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
          { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1182 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 89: // instance: attr_instance_seq ID ID '(' inst_pins ')' ';'
#line 392 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeModuleInst((yystack_[5].value.string), (yystack_[4].value.string), (yystack_[2].value.nets), (yystack_[6].value.attr_stmt_seq), loc_line(yystack_[5].location)); }
#line 1188 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 90: // instance: attr_instance_seq ID parameter_values ID '(' inst_pins ')' ';'
#line 394 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.stmt) = reader->makeModuleInst((yystack_[6].value.string), (yystack_[4].value.string), (yystack_[2].value.nets), (yystack_[7].value.attr_stmt_seq), loc_line(yystack_[6].location)); }
#line 1194 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 92: // parameter_exprs: parameter_expr
#line 402 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[0].value.ival); }
#line 1200 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 93: // parameter_exprs: '{' parameter_exprs '}'
#line 404 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[1].value.ival); }
#line 1206 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 94: // parameter_exprs: parameter_exprs ',' parameter_expr
#line 405 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.ival) = (yystack_[2].value.ival); }
#line 1212 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 95: // inst_pins: %empty
#line 410 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = nullptr; }
#line 1218 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 96: // inst_pins: inst_ordered_pins
#line 411 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = (yystack_[0].value.nets); }
#line 1224 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 97: // inst_pins: inst_named_pins
#line 412 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = (yystack_[0].value.nets); }
#line 1230 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 98: // inst_ordered_pins: net_expr
#line 418 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = new sta::VerilogNetSeq;
	  (yylhs.value.nets)->push_back((yystack_[0].value.net));
	}
#line 1238 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 99: // inst_ordered_pins: inst_ordered_pins ',' net_expr
#line 422 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.nets)->push_back((yystack_[0].value.net)); }
#line 1244 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 100: // inst_named_pins: inst_named_pin
#line 428 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = new sta::VerilogNetSeq;
	  (yylhs.value.nets)->push_back((yystack_[0].value.net));
	}
#line 1252 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 101: // inst_named_pins: inst_named_pins ',' inst_named_pin
#line 432 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yystack_[2].value.nets)->push_back((yystack_[0].value.net)); }
#line 1258 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 102: // inst_named_pin: '.' ID '(' ')'
#line 440 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefScalarNet((yystack_[2].value.string)); }
#line 1264 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 103: // inst_named_pin: '.' ID '(' ID ')'
#line 442 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefScalarNet((yystack_[3].value.string), (yystack_[1].value.string)); }
#line 1270 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 104: // inst_named_pin: '.' ID '(' ID '[' INT ']' ')'
#line 444 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefBitSelect((yystack_[6].value.string), (yystack_[4].value.string), (yystack_[2].value.ival)); }
#line 1276 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 105: // inst_named_pin: '.' ID '(' named_pin_net_expr ')'
#line 446 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefScalar((yystack_[3].value.string), (yystack_[1].value.net)); }
#line 1282 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 106: // inst_named_pin: '.' ID '[' INT ']' '(' ')'
#line 449 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefBit((yystack_[5].value.string), (yystack_[3].value.ival), nullptr); }
#line 1288 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 107: // inst_named_pin: '.' ID '[' INT ']' '(' net_expr ')'
#line 451 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefBit((yystack_[6].value.string), (yystack_[4].value.ival), (yystack_[1].value.net)); }
#line 1294 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 108: // inst_named_pin: '.' ID '[' INT ':' INT ']' '(' ')'
#line 454 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefPart((yystack_[7].value.string), (yystack_[5].value.ival), (yystack_[3].value.ival), nullptr); }
#line 1300 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 109: // inst_named_pin: '.' ID '[' INT ':' INT ']' '(' net_expr ')'
#line 456 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetNamedPortRefPart((yystack_[8].value.string), (yystack_[6].value.ival), (yystack_[4].value.ival), (yystack_[1].value.net)); }
#line 1306 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 110: // named_pin_net_expr: net_part_select
#line 460 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1312 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 111: // named_pin_net_expr: net_constant
#line 461 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1318 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 112: // named_pin_net_expr: net_expr_concat
#line 462 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1324 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 113: // net_named: net_scalar
#line 466 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1330 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 114: // net_named: net_bit_select
#line 467 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1336 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 115: // net_named: net_part_select
#line 468 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1342 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 116: // net_scalar: ID
#line 473 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetScalar((yystack_[0].value.string)); }
#line 1348 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 117: // net_bit_select: ID '[' INT ']'
#line 478 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetBitSelect((yystack_[3].value.string), (yystack_[1].value.ival)); }
#line 1354 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 118: // net_part_select: ID '[' INT ':' INT ']'
#line 483 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetPartSelect((yystack_[5].value.string), (yystack_[3].value.ival), (yystack_[1].value.ival)); }
#line 1360 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 119: // net_constant: CONSTANT
#line 488 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetConstant((yystack_[0].value.string), loc_line(yystack_[0].location)); }
#line 1366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 120: // net_expr_concat: '{' net_exprs '}'
#line 493 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = reader->makeNetConcat((yystack_[1].value.nets)); }
#line 1372 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 121: // net_exprs: net_expr
#line 498 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets) = new sta::VerilogNetSeq;
	  (yylhs.value.nets)->push_back((yystack_[0].value.net));
	}
#line 1380 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 122: // net_exprs: net_exprs ',' net_expr
#line 502 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.nets)->push_back((yystack_[0].value.net)); }
#line 1386 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 123: // net_expr: net_scalar
#line 506 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1392 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 124: // net_expr: net_bit_select
#line 507 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1398 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 125: // net_expr: net_part_select
#line 508 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1404 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 126: // net_expr: net_constant
#line 509 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1410 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 127: // net_expr: net_expr_concat
#line 510 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.net) = (yystack_[0].value.net); }
#line 1416 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 128: // attr_instance_seq: %empty
#line 515 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_stmt_seq) = new sta::VerilogAttrStmtSeq; }
#line 1422 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 129: // attr_instance_seq: attr_instance_seq attr_instance
#line 517 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { if ((yystack_[0].value.attr_stmt)) (yystack_[1].value.attr_stmt_seq)->push_back((yystack_[0].value.attr_stmt)); }
#line 1428 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 130: // attr_instance: ATTR_OPEN attr_specs ATTR_CLOSED
#line 522 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_stmt) = new sta::VerilogAttrStmt((yystack_[1].value.attr_seq)); }
#line 1434 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 131: // attr_specs: attr_spec
#line 527 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_seq) = new sta::VerilogAttrEntrySeq;
	  (yylhs.value.attr_seq)->push_back((yystack_[0].value.attr_entry));
	}
#line 1442 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 132: // attr_specs: attr_specs ',' attr_spec
#line 531 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_seq)->push_back((yystack_[0].value.attr_entry)); }
#line 1448 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 133: // attr_spec: ID
#line 536 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_entry) = new sta::VerilogAttrEntry(*(yystack_[0].value.string), "1"); delete (yystack_[0].value.string); }
#line 1454 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 134: // attr_spec: ID '=' attr_spec_value
#line 538 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.attr_entry) = new sta::VerilogAttrEntry(*(yystack_[2].value.string), *(yystack_[0].value.string)); delete (yystack_[2].value.string); delete (yystack_[0].value.string); }
#line 1460 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 135: // attr_spec_value: CONSTANT
#line 543 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1466 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 136: // attr_spec_value: STRING
#line 545 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1472 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;

  case 137: // attr_spec_value: INT
#line 547 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
        { (yylhs.value.string) = new std::string(std::to_string((yystack_[0].value.ival))); }
#line 1478 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"
    break;


#line 1482 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"

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
  VerilogParse::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  VerilogParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const signed char VerilogParse::yypact_ninf_ = -126;

  const short VerilogParse::yytable_ninf_ = -129;

  const short
  VerilogParse::yypact_[] =
  {
    -126,    28,    79,  -126,  -126,    16,    76,    78,  -126,     0,
      66,    -4,  -126,  -126,    73,   116,  -126,    78,   201,   120,
     104,   124,   156,   111,  -126,  -126,  -126,   119,  -126,  -126,
    -126,  -126,   208,  -126,  -126,  -126,  -126,  -126,   131,  -126,
      22,    19,   167,   160,  -126,  -126,  -126,  -126,  -126,  -126,
    -126,  -126,   162,   202,  -126,   192,    61,  -126,   198,    15,
     205,    90,   224,    14,    24,    86,  -126,    32,    99,  -126,
     268,  -126,  -126,  -126,  -126,  -126,   274,   244,   159,  -126,
     275,   161,  -126,   238,   276,    10,  -126,  -126,  -126,  -126,
    -126,  -126,  -126,  -126,  -126,    89,   174,   240,   113,   156,
    -126,  -126,  -126,  -126,   158,  -126,  -126,  -126,  -126,  -126,
    -126,  -126,  -126,   315,  -126,  -126,  -126,  -126,  -126,  -126,
    -126,   169,  -126,  -126,    22,    32,    71,   279,  -126,   238,
      97,  -126,   167,   288,  -126,   238,   289,   290,   316,   321,
     199,  -126,   322,  -126,  -126,  -126,   292,  -126,   263,   286,
     287,    32,  -126,  -126,  -126,  -126,  -126,  -126,  -126,    39,
      39,   323,   285,   326,  -126,  -126,   285,  -126,  -126,   298,
     112,    13,   299,   291,  -126,    90,   293,  -126,  -126,  -126,
     330,  -126,  -126,   141,  -126,    39,    39,    39,    39,   294,
    -126,   331,   303,   304,   305,  -126,  -126,    13,   285,   219,
     112,   335,  -126,  -126,   300,  -126,   247,   247,  -126,  -126,
     238,   118,   310,    32,   307,   197,  -126,    39,   311,   306,
      90,   234,   101,   341,  -126,  -126,  -126,  -126,   285,   314,
      90,  -126,  -126,    -9,  -126,   317,  -126,  -126,  -126,   252,
    -126,   235,  -126,   345,  -126,   346,   319,  -126,   259,   309,
     128,   320,   324,  -126,   325,  -126,   139,  -126,  -126,   327,
    -126
  };

  const unsigned char
  VerilogParse::yydefact_[] =
  {
       3,     0,   128,     1,     4,     0,     0,     0,   129,     0,
     133,     0,   131,    34,   128,     0,   130,     0,     0,   116,
       0,     0,     0,     0,     9,    11,    14,     0,    21,    18,
      19,    20,     0,   137,   135,   136,   134,   132,     0,     5,
       0,     0,     0,     0,    35,    36,    41,    37,    38,    39,
      43,    40,     0,     0,    34,     0,     0,    16,     0,     0,
       0,   128,    26,    31,    28,     0,    42,     0,     0,    84,
       0,    87,   113,   114,   115,    88,     0,     0,     0,    49,
       0,     0,    64,     0,     0,     0,    77,    76,    78,    75,
      70,    72,    71,    74,    73,     0,     0,     0,     0,     0,
      15,    34,    10,    34,    81,    22,    23,    82,    27,    32,
      33,    30,    29,     0,    24,   119,   123,   124,   125,   126,
     127,     0,   121,    83,     0,     0,     0,     0,    47,     0,
       0,    63,     0,     0,    44,     0,     0,     0,     0,     0,
       0,    79,     0,   117,     6,    12,     0,    17,     0,     0,
       0,     0,   120,    85,    86,    56,    55,    53,    52,     0,
       0,     0,    51,     0,    50,    67,    66,    65,    45,     0,
      95,     0,     0,     0,    68,     0,     0,    13,     7,     8,
       0,   122,    57,     0,    54,     0,     0,     0,     0,     0,
      46,     0,     0,    96,    97,   100,    98,     0,    92,     0,
      95,     0,    80,   118,     0,    62,    59,    58,    60,    61,
       0,     0,     0,     0,     0,     0,    91,     0,     0,     0,
       0,     0,     0,     0,    89,    99,   101,    93,    94,     0,
       0,    25,    48,     0,   102,     0,   110,   111,   112,     0,
      90,     0,   103,     0,   105,     0,     0,    69,     0,     0,
       0,     0,     0,   106,     0,   104,     0,   107,   108,     0,
     109
  };

  const short
  VerilogParse::yypgoto_[] =
  {
    -126,  -126,  -126,  -126,  -126,   296,   255,  -126,   -19,  -126,
     295,  -126,   -33,  -126,  -126,  -126,  -126,  -126,   148,   -64,
    -125,  -126,  -126,   228,  -126,  -126,   132,   -54,  -126,  -126,
     -27,  -126,  -126,  -126,   166,   164,  -126,  -126,   151,  -126,
    -126,   -12,   -10,   -14,   144,   -58,  -126,   -61,    12,  -126,
    -126,   350,  -126
  };

  const unsigned char
  VerilogParse::yydefgoto_[] =
  {
       0,     1,     2,     4,    23,    24,    25,    56,    26,    27,
      28,    65,    18,    44,    45,    46,    84,    47,    78,    79,
     198,    48,    81,    82,    49,    95,   140,   141,    50,    68,
     107,    70,    51,   138,   199,   192,   193,   194,   195,   235,
      71,    72,    73,    74,   119,    75,   121,   196,    52,     8,
      11,    12,    36
  };

  const short
  VerilogParse::yytable_[] =
  {
      31,   162,    29,    57,    30,   166,   122,   106,    31,   120,
      29,   114,    30,    69,     5,   136,   155,   156,   157,   133,
      19,    97,    16,     6,    76,   242,    32,    19,     3,   109,
     243,    17,    13,    14,   182,   183,   115,    19,   110,   111,
     159,     7,   155,   156,   157,    31,   160,    29,   112,    30,
     197,    21,    22,   118,   137,   116,   161,   117,    77,    67,
     206,   207,   208,   209,   154,   164,   159,   120,   148,    67,
     149,   169,   160,    32,   155,   156,   157,   158,    19,    -2,
     147,     9,   161,    10,    31,    31,    29,    29,    30,    30,
     181,   104,   228,   120,   104,   104,    99,   153,   159,   100,
     155,   156,   157,   165,   160,   115,   233,    20,    15,    21,
      22,   118,   120,   116,   161,   117,   115,    19,    19,    33,
      34,   202,    35,    67,   159,   113,    67,    67,   139,    55,
     160,   123,   115,    19,   124,   234,    54,   118,    67,   116,
     161,   117,   120,   115,    19,    58,    59,   145,   191,    67,
      22,   222,   225,    60,    61,   120,   118,   223,   116,    53,
     117,    19,   253,    66,   238,    67,   231,    85,   185,   186,
     187,   188,    80,   258,    83,   205,    67,    86,    87,    88,
      89,    90,    91,    92,    93,    94,   118,     7,   116,   254,
     117,   128,   120,   131,   129,   259,   132,    53,   120,   118,
    -116,   116,    38,   117,   151,    96,  -128,   152,   236,    39,
      40,    41,    42,    43,   142,   143,  -128,  -128,  -128,  -128,
    -128,  -128,  -128,  -128,  -128,    98,  -128,    62,    63,    64,
     101,   174,   217,     7,   175,   227,   118,   103,   116,   108,
     117,    38,   118,    76,   116,  -128,   117,   127,   144,    40,
      41,    42,    43,   216,   217,  -128,  -128,  -128,  -128,  -128,
    -128,  -128,  -128,  -128,    38,  -128,   232,   247,  -128,   129,
     175,   178,    40,    41,    42,    43,   187,   188,  -128,  -128,
    -128,  -128,  -128,  -128,  -128,  -128,  -128,    38,  -128,   134,
     135,  -128,   245,   246,   179,    40,    41,    42,    43,   142,
     251,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,  -128,
     125,  -128,   185,   186,   187,   188,   126,   130,   150,   163,
     168,   172,   170,   171,   173,   176,   177,   180,   184,   189,
     190,   201,   200,   204,   203,   210,   211,   212,   219,   213,
     214,   220,   224,   191,   239,   229,   240,   230,   248,   249,
     252,   244,   250,   146,   255,   102,   105,   256,   221,   257,
     167,   260,   241,   215,   218,   226,   237,    37
  };

  const short
  VerilogParse::yycheck_[] =
  {
      14,   126,    14,    22,    14,   130,    67,    61,    22,    67,
      22,    65,    22,    40,     2,     5,     3,     4,     5,    83,
       5,    54,    26,     7,     5,    34,    14,     5,     0,    15,
      39,    35,    32,    33,   159,   160,     4,     5,    24,    15,
      27,    25,     3,     4,     5,    59,    33,    59,    24,    59,
      37,    36,    37,    67,    44,    67,    43,    67,    39,    37,
     185,   186,   187,   188,   125,   129,    27,   125,   101,    37,
     103,   135,    33,    61,     3,     4,     5,     6,     5,     0,
      99,     5,    43,     5,    98,    99,    98,    99,    98,    99,
     151,     5,   217,   151,     5,     5,    35,   124,    27,    38,
       3,     4,     5,     6,    33,     4,     5,    34,    42,    36,
      37,   125,   170,   125,    43,   125,     4,     5,     5,     3,
       4,   175,     6,    37,    27,    39,    37,    37,    39,     5,
      33,    32,     4,     5,    35,    34,    32,   151,    37,   151,
      43,   151,   200,     4,     5,    34,    35,    34,    36,    37,
      37,    33,   213,    34,    35,   213,   170,    39,   170,    39,
     170,     5,    34,    32,   222,    37,   220,     5,    27,    28,
      29,    30,     5,    34,    14,    34,    37,    15,    16,    17,
      18,    19,    20,    21,    22,    23,   200,    25,   200,   250,
     200,    32,   250,    32,    35,   256,    35,    39,   256,   213,
      42,   213,     1,   213,    35,     3,     5,    38,   222,     8,
       9,    10,    11,    12,    40,    41,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    33,    25,    19,    20,    21,
      32,    32,    35,    25,    35,    38,   250,    32,   250,    15,
     250,     1,   256,     5,   256,     5,   256,     3,     8,     9,
      10,    11,    12,    34,    35,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     1,    25,    32,    32,     5,    35,
      35,     8,     9,    10,    11,    12,    29,    30,    15,    16,
      17,    18,    19,    20,    21,    22,    23,     1,    25,    13,
      14,     5,    40,    41,     8,     9,    10,    11,    12,    40,
      41,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      42,    25,    27,    28,    29,    30,    42,    42,     3,    40,
      32,     5,    33,    33,     3,     3,    34,    40,     5,     3,
      32,    40,    33,     3,    41,    41,     5,    34,     3,    35,
      35,    41,    32,    36,     3,    34,    32,    41,     3,     3,
      41,    34,    33,    98,    34,    59,    61,    33,   210,    34,
     132,    34,   230,   197,   200,   214,   222,    17
  };

  const signed char
  VerilogParse::yystos_[] =
  {
       0,    46,    47,     0,    48,    93,     7,    25,    94,     5,
       5,    95,    96,    32,    33,    42,    26,    35,    57,     5,
      34,    36,    37,    49,    50,    51,    53,    54,    55,    86,
      87,    88,    93,     3,     4,     6,    97,    96,     1,     8,
       9,    10,    11,    12,    58,    59,    60,    62,    66,    69,
      73,    77,    93,    39,    32,     5,    52,    53,    34,    35,
      34,    35,    19,    20,    21,    56,    32,    37,    74,    75,
      76,    85,    86,    87,    88,    90,     5,    39,    63,    64,
       5,    67,    68,    14,    61,     5,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    70,     3,    57,    33,    35,
      38,    32,    50,    32,     5,    55,    72,    75,    15,    15,
      24,    15,    24,    39,    72,     4,    86,    87,    88,    89,
      90,    91,    92,    32,    35,    42,    42,     3,    32,    35,
      42,    32,    35,    64,    13,    14,     5,    44,    78,    39,
      71,    72,    40,    41,     8,    34,    51,    53,    57,    57,
       3,    35,    38,    75,    92,     3,     4,     5,     6,    27,
      33,    43,    65,    40,    64,     6,    65,    68,    32,    64,
      33,    33,     5,     3,    32,    35,     3,    34,     8,     8,
      40,    92,    65,    65,     5,    27,    28,    29,    30,     3,
      32,    36,    80,    81,    82,    83,    92,    37,    65,    79,
      33,    40,    72,    41,     3,    34,    65,    65,    65,    65,
      41,     5,    34,    35,    35,    79,    34,    35,    80,     3,
      41,    63,    33,    39,    32,    92,    83,    38,    65,    34,
      41,    72,    32,     5,    34,    84,    88,    89,    90,     3,
      32,    71,    34,    39,    34,    40,    41,    32,     3,     3,
      33,    41,    41,    34,    92,    34,    33,    34,    34,    92,
      34
  };

  const signed char
  VerilogParse::yyr1_[] =
  {
       0,    45,    46,    47,    47,    48,    48,    48,    48,    49,
      49,    50,    50,    50,    51,    51,    52,    52,    53,    53,
      53,    54,    54,    54,    55,    55,    56,    56,    56,    56,
      56,    56,    56,    56,    57,    57,    57,    58,    58,    58,
      58,    58,    58,    59,    60,    61,    61,    62,    62,    63,
      63,    64,    64,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    66,    67,    67,    68,    68,    69,    69,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    71,
      71,    72,    72,    73,    74,    74,    75,    76,    76,    77,
      77,    78,    79,    79,    79,    80,    80,    80,    81,    81,
      82,    82,    83,    83,    83,    83,    83,    83,    83,    83,
      84,    84,    84,    85,    85,    85,    86,    87,    88,    89,
      90,    91,    91,    92,    92,    92,    92,    92,    93,    93,
      94,    95,    95,    96,    96,    97,    97,    97
  };

  const signed char
  VerilogParse::yyr2_[] =
  {
       0,     2,     1,     0,     2,     6,     8,     9,     9,     1,
       3,     1,     4,     5,     1,     3,     1,     3,     1,     1,
       1,     1,     3,     3,     3,     8,     1,     2,     1,     2,
       2,     1,     2,     2,     0,     2,     2,     1,     1,     1,
       1,     1,     2,     1,     3,     3,     4,     3,     8,     1,
       3,     3,     3,     1,     2,     1,     1,     2,     3,     3,
       3,     3,     3,     3,     1,     3,     3,     3,     4,     9,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     3,     1,     3,     3,     1,     1,     7,
       8,     4,     1,     3,     3,     0,     1,     1,     1,     3,
       1,     3,     4,     5,     8,     5,     7,     8,     9,    10,
       1,     1,     1,     1,     1,     1,     1,     4,     6,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     0,     2,
       3,     1,     3,     1,     3,     1,     1,     1
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const VerilogParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "INT", "CONSTANT",
  "ID", "STRING", "MODULE", "ENDMODULE", "ASSIGN", "PARAMETER", "DEFPARAM",
  "SPECIFY", "ENDSPECIFY", "SPECPARAM", "WIRE", "WAND", "WOR", "TRI",
  "INPUT", "OUTPUT", "INOUT", "SUPPLY1", "SUPPLY0", "REG", "ATTR_OPEN",
  "ATTR_CLOSED", "'-'", "'+'", "'*'", "'/'", "NEG", "';'", "'('", "')'",
  "','", "'.'", "'{'", "'}'", "'['", "':'", "']'", "'='", "'`'", "'#'",
  "$accept", "file", "modules", "module", "port_list", "port", "port_expr",
  "port_refs", "port_ref", "port_dcls", "port_dcl", "port_dcl_type",
  "stmts", "stmt", "stmt_seq", "specify_block", "specify_stmts",
  "parameter", "parameter_dcls", "parameter_dcl", "parameter_expr",
  "defparam", "param_values", "param_value", "declaration", "dcl_type",
  "dcl_args", "dcl_arg", "continuous_assign", "net_assignments",
  "net_assignment", "net_assign_lhs", "instance", "parameter_values",
  "parameter_exprs", "inst_pins", "inst_ordered_pins", "inst_named_pins",
  "inst_named_pin", "named_pin_net_expr", "net_named", "net_scalar",
  "net_bit_select", "net_part_select", "net_constant", "net_expr_concat",
  "net_exprs", "net_expr", "attr_instance_seq", "attr_instance",
  "attr_specs", "attr_spec", "attr_spec_value", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  VerilogParse::yyrline_[] =
  {
       0,   128,   128,   131,   133,   137,   139,   141,   143,   148,
     152,   157,   158,   160,   165,   166,   170,   174,   179,   180,
     181,   185,   189,   193,   202,   204,   209,   210,   211,   212,
     213,   214,   215,   216,   221,   222,   224,   234,   235,   236,
     237,   238,   239,   244,   255,   260,   262,   268,   270,   275,
     277,   282,   284,   289,   291,   293,   295,   296,   298,   300,
     302,   304,   306,   311,   316,   318,   323,   325,   330,   332,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   349,
     353,   360,   362,   367,   372,   376,   381,   386,   387,   391,
     393,   398,   402,   403,   405,   410,   411,   412,   417,   421,
     427,   431,   439,   441,   443,   445,   448,   450,   453,   455,
     460,   461,   462,   466,   467,   468,   472,   477,   482,   487,
     492,   497,   501,   506,   507,   508,   509,   510,   515,   516,
     521,   526,   530,   535,   537,   542,   544,   546
  };

  void
  VerilogParse::yy_stack_print_ () const
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
  VerilogParse::yy_reduce_print_ (int yyrule) const
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

  VerilogParse::symbol_kind_type
  VerilogParse::yytranslate_ (int t) YY_NOEXCEPT
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
       2,     2,     2,     2,     2,    44,     2,     2,     2,     2,
      33,    34,    29,    28,    35,    27,    36,    30,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    40,    32,
       2,    42,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    39,     2,    41,     2,     2,    43,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    37,     2,    38,     2,     2,     2,     2,
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
      25,    26,    31
    };
    // Last valid token kind.
    const int code_max = 282;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 55 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"
} // sta
#line 2047 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/VerilogParse.cc"

#line 550 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/verilog/VerilogParse.yy"

