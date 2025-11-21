// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file /home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.hh
 ** Define the sta::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_STA_SPEFPARSE_HH_INCLUDED
# define YY_YY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_STA_SPEFPARSE_HH_INCLUDED

# include <cassert>
# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "SpefLocation.hh"


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
namespace sta {
#line 183 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.hh"




  /// A Bison parser.
  class SpefParse
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
    /// Symbol semantic values.
    union value_type
    {
#line 60 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"

  char ch;
  char *string;
  int integer;
  float number;
  sta::StringSeq *string_seq;
  sta::PortDirection *port_dir;
  sta::SpefRspfPi *pi;
  sta::SpefTriple *triple;
  sta::Pin *pin;
  sta::Net *net;

#line 214 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.hh"

    };
#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;

    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    YYEOF = 0,                     // "end of file"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    SPEF = 258,                    // SPEF
    DESIGN = 259,                  // DESIGN
    DATE = 260,                    // DATE
    VENDOR = 261,                  // VENDOR
    PROGRAM = 262,                 // PROGRAM
    DESIGN_FLOW = 263,             // DESIGN_FLOW
    PVERSION = 264,                // PVERSION
    DIVIDER = 265,                 // DIVIDER
    DELIMITER = 266,               // DELIMITER
    BUS_DELIMITER = 267,           // BUS_DELIMITER
    T_UNIT = 268,                  // T_UNIT
    C_UNIT = 269,                  // C_UNIT
    R_UNIT = 270,                  // R_UNIT
    L_UNIT = 271,                  // L_UNIT
    NAME_MAP = 272,                // NAME_MAP
    POWER_NETS = 273,              // POWER_NETS
    GROUND_NETS = 274,             // GROUND_NETS
    KW_C = 275,                    // KW_C
    KW_L = 276,                    // KW_L
    KW_S = 277,                    // KW_S
    KW_D = 278,                    // KW_D
    KW_V = 279,                    // KW_V
    PORTS = 280,                   // PORTS
    PHYSICAL_PORTS = 281,          // PHYSICAL_PORTS
    DEFINE = 282,                  // DEFINE
    PDEFINE = 283,                 // PDEFINE
    D_NET = 284,                   // D_NET
    D_PNET = 285,                  // D_PNET
    R_NET = 286,                   // R_NET
    R_PNET = 287,                  // R_PNET
    END = 288,                     // END
    CONN = 289,                    // CONN
    CAP = 290,                     // CAP
    RES = 291,                     // RES
    INDUC = 292,                   // INDUC
    KW_P = 293,                    // KW_P
    KW_I = 294,                    // KW_I
    KW_N = 295,                    // KW_N
    DRIVER = 296,                  // DRIVER
    CELL = 297,                    // CELL
    C2_R1_C1 = 298,                // C2_R1_C1
    LOADS = 299,                   // LOADS
    RC = 300,                      // RC
    KW_Q = 301,                    // KW_Q
    KW_K = 302,                    // KW_K
    INTEGER = 303,                 // INTEGER
    FLOAT = 304,                   // FLOAT
    QSTRING = 305,                 // QSTRING
    INDEX = 306,                   // INDEX
    IDENT = 307,                   // IDENT
    NAME = 308                     // NAME
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 66, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_SPEF = 3,                              // SPEF
        S_DESIGN = 4,                            // DESIGN
        S_DATE = 5,                              // DATE
        S_VENDOR = 6,                            // VENDOR
        S_PROGRAM = 7,                           // PROGRAM
        S_DESIGN_FLOW = 8,                       // DESIGN_FLOW
        S_PVERSION = 9,                          // PVERSION
        S_DIVIDER = 10,                          // DIVIDER
        S_DELIMITER = 11,                        // DELIMITER
        S_BUS_DELIMITER = 12,                    // BUS_DELIMITER
        S_T_UNIT = 13,                           // T_UNIT
        S_C_UNIT = 14,                           // C_UNIT
        S_R_UNIT = 15,                           // R_UNIT
        S_L_UNIT = 16,                           // L_UNIT
        S_NAME_MAP = 17,                         // NAME_MAP
        S_POWER_NETS = 18,                       // POWER_NETS
        S_GROUND_NETS = 19,                      // GROUND_NETS
        S_KW_C = 20,                             // KW_C
        S_KW_L = 21,                             // KW_L
        S_KW_S = 22,                             // KW_S
        S_KW_D = 23,                             // KW_D
        S_KW_V = 24,                             // KW_V
        S_PORTS = 25,                            // PORTS
        S_PHYSICAL_PORTS = 26,                   // PHYSICAL_PORTS
        S_DEFINE = 27,                           // DEFINE
        S_PDEFINE = 28,                          // PDEFINE
        S_D_NET = 29,                            // D_NET
        S_D_PNET = 30,                           // D_PNET
        S_R_NET = 31,                            // R_NET
        S_R_PNET = 32,                           // R_PNET
        S_END = 33,                              // END
        S_CONN = 34,                             // CONN
        S_CAP = 35,                              // CAP
        S_RES = 36,                              // RES
        S_INDUC = 37,                            // INDUC
        S_KW_P = 38,                             // KW_P
        S_KW_I = 39,                             // KW_I
        S_KW_N = 40,                             // KW_N
        S_DRIVER = 41,                           // DRIVER
        S_CELL = 42,                             // CELL
        S_C2_R1_C1 = 43,                         // C2_R1_C1
        S_LOADS = 44,                            // LOADS
        S_RC = 45,                               // RC
        S_KW_Q = 46,                             // KW_Q
        S_KW_K = 47,                             // KW_K
        S_INTEGER = 48,                          // INTEGER
        S_FLOAT = 49,                            // FLOAT
        S_QSTRING = 50,                          // QSTRING
        S_INDEX = 51,                            // INDEX
        S_IDENT = 52,                            // IDENT
        S_NAME = 53,                             // NAME
        S_54_ = 54,                              // '['
        S_55_ = 55,                              // '{'
        S_56_ = 56,                              // '('
        S_57_ = 57,                              // '<'
        S_58_ = 58,                              // ']'
        S_59_ = 59,                              // '}'
        S_60_ = 60,                              // ')'
        S_61_ = 61,                              // '>'
        S_62_ = 62,                              // '.'
        S_63_ = 63,                              // '/'
        S_64_ = 64,                              // '|'
        S_65_ = 65,                              // ':'
        S_YYACCEPT = 66,                         // $accept
        S_file = 67,                             // file
        S_prefix_bus_delim = 68,                 // prefix_bus_delim
        S_suffix_bus_delim = 69,                 // suffix_bus_delim
        S_hchar = 70,                            // hchar
        S_header_def = 71,                       // header_def
        S_spef_version = 72,                     // spef_version
        S_design_name = 73,                      // design_name
        S_date = 74,                             // date
        S_program_name = 75,                     // program_name
        S_program_version = 76,                  // program_version
        S_vendor = 77,                           // vendor
        S_design_flow = 78,                      // design_flow
        S_qstrings = 79,                         // qstrings
        S_hierarchy_div_def = 80,                // hierarchy_div_def
        S_pin_delim_def = 81,                    // pin_delim_def
        S_bus_delim_def = 82,                    // bus_delim_def
        S_unit_def = 83,                         // unit_def
        S_time_scale = 84,                       // time_scale
        S_cap_scale = 85,                        // cap_scale
        S_res_scale = 86,                        // res_scale
        S_induc_scale = 87,                      // induc_scale
        S_name_map = 88,                         // name_map
        S_name_map_entries = 89,                 // name_map_entries
        S_name_map_entry = 90,                   // name_map_entry
        S_mapped_item = 91,                      // mapped_item
        S_power_def = 92,                        // power_def
        S_power_net_def = 93,                    // power_net_def
        S_ground_net_def = 94,                   // ground_net_def
        S_net_names = 95,                        // net_names
        S_net_name = 96,                         // net_name
        S_external_def = 97,                     // external_def
        S_port_def = 98,                         // port_def
        S_port_entries = 99,                     // port_entries
        S_port_entry = 100,                      // port_entry
        S_direction = 101,                       // direction
        S_port_name = 102,                       // port_name
        S_inst_name = 103,                       // inst_name
        S_physical_port_def = 104,               // physical_port_def
        S_pport_entries = 105,                   // pport_entries
        S_pport_entry = 106,                     // pport_entry
        S_pport_name = 107,                      // pport_name
        S_pport = 108,                           // pport
        S_physical_inst = 109,                   // physical_inst
        S_conn_attrs = 110,                      // conn_attrs
        S_conn_attr = 111,                       // conn_attr
        S_coordinates = 112,                     // coordinates
        S_cap_load = 113,                        // cap_load
        S_par_value = 114,                       // par_value
        S_slews = 115,                           // slews
        S_threshold = 116,                       // threshold
        S_driving_cell = 117,                    // driving_cell
        S_cell_type = 118,                       // cell_type
        S_define_def = 119,                      // define_def
        S_define_entry = 120,                    // define_entry
        S_entity = 121,                          // entity
        S_internal_def = 122,                    // internal_def
        S_nets = 123,                            // nets
        S_d_net = 124,                           // d_net
        S_125_1 = 125,                           // $@1
        S_net = 126,                             // net
        S_total_cap = 127,                       // total_cap
        S_routing_conf = 128,                    // routing_conf
        S_conf = 129,                            // conf
        S_conn_sec = 130,                        // conn_sec
        S_conn_defs = 131,                       // conn_defs
        S_conn_def = 132,                        // conn_def
        S_external_connection = 133,             // external_connection
        S_internal_connection = 134,             // internal_connection
        S_pin_name = 135,                        // pin_name
        S_internal_node_coords = 136,            // internal_node_coords
        S_internal_node_coord = 137,             // internal_node_coord
        S_internal_parasitic_node = 138,         // internal_parasitic_node
        S_cap_sec = 139,                         // cap_sec
        S_cap_elems = 140,                       // cap_elems
        S_cap_elem = 141,                        // cap_elem
        S_cap_id = 142,                          // cap_id
        S_parasitic_node = 143,                  // parasitic_node
        S_res_sec = 144,                         // res_sec
        S_res_elems = 145,                       // res_elems
        S_res_elem = 146,                        // res_elem
        S_res_id = 147,                          // res_id
        S_induc_sec = 148,                       // induc_sec
        S_induc_elems = 149,                     // induc_elems
        S_induc_elem = 150,                      // induc_elem
        S_induc_id = 151,                        // induc_id
        S_r_net = 152,                           // r_net
        S_153_2 = 153,                           // $@2
        S_driver_reducs = 154,                   // driver_reducs
        S_driver_reduc = 155,                    // driver_reduc
        S_156_3 = 156,                           // $@3
        S_driver_pair = 157,                     // driver_pair
        S_driver_cell = 158,                     // driver_cell
        S_pi_model = 159,                        // pi_model
        S_load_desc = 160,                       // load_desc
        S_rc_descs = 161,                        // rc_descs
        S_rc_desc = 162,                         // rc_desc
        S_pole_residue_desc = 163,               // pole_residue_desc
        S_pole_desc = 164,                       // pole_desc
        S_poles = 165,                           // poles
        S_pole = 166,                            // pole
        S_complex_par_value = 167,               // complex_par_value
        S_cnumber = 168,                         // cnumber
        S_real_component = 169,                  // real_component
        S_imaginary_component = 170,             // imaginary_component
        S_residue_desc = 171,                    // residue_desc
        S_residues = 172,                        // residues
        S_residue = 173,                         // residue
        S_d_pnet = 174,                          // d_pnet
        S_pnet_ref = 175,                        // pnet_ref
        S_pconn_sec = 176,                       // pconn_sec
        S_pconn_defs = 177,                      // pconn_defs
        S_pconn_def = 178,                       // pconn_def
        S_pexternal_connection = 179,            // pexternal_connection
        S_internal_pnode_coords = 180,           // internal_pnode_coords
        S_internal_pnode_coord = 181,            // internal_pnode_coord
        S_internal_pdspf_node = 182,             // internal_pdspf_node
        S_name_or_index = 183,                   // name_or_index
        S_r_pnet = 184,                          // r_pnet
        S_pdriver_reduc = 185,                   // pdriver_reduc
        S_pdriver_pair = 186,                    // pdriver_pair
        S_number = 187,                          // number
        S_pos_integer = 188,                     // pos_integer
        S_pos_number = 189                       // pos_number
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value (std::move (that.value))
        , location (std::move (that.location))
      {}
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);
      /// Constructor for valueless symbols.
      basic_symbol (typename Base::kind_type t,
                    YY_MOVE_REF (location_type) l);

      /// Constructor for symbols with semantic value.
      basic_symbol (typename Base::kind_type t,
                    YY_RVREF (value_type) v,
                    YY_RVREF (location_type) l);

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        Base::clear ();
      }

#if YYDEBUG || 0
      /// The user-facing name of this symbol.
      const char *name () const YY_NOEXCEPT
      {
        return SpefParse::symbol_name (this->kind ());
      }
#endif // #if YYDEBUG || 0


      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {};

    /// Build a parser object.
    SpefParse (SpefScanner *scanner_yyarg, SpefReader *reader_yyarg);
    virtual ~SpefParse ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    SpefParse (const SpefParse&) = delete;
    /// Non copyable.
    SpefParse& operator= (const SpefParse&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

#if YYDEBUG || 0
    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static const char *symbol_name (symbol_kind_type yysymbol);
#endif // #if YYDEBUG || 0




  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    SpefParse (const SpefParse&);
    /// Non copyable.
    SpefParse& operator= (const SpefParse&);
#endif


    /// Stored state numbers (used for stacks).
    typedef short state_type;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const short yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

#if YYDEBUG || 0
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif // #if YYDEBUG || 0


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const unsigned char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const short yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const unsigned char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const unsigned char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 253,     ///< Last index in yytable_.
      yynnts_ = 124,  ///< Number of nonterminal symbols.
      yyfinal_ = 6 ///< Termination state number.
    };


    // User arguments.
    SpefScanner *scanner;
    SpefReader *reader;

  };


#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
} // sta
#line 1016 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.hh"




#endif // !YY_YY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_STA_SPEFPARSE_HH_INCLUDED
