/*! pre-defined static diagnostic IDs */
enum class diag_id
{
  /* general */
  ERR_FILE_OPEN,
  ERR_PARSE_LINE,
  WRN_UNRESOLVED_DEPENDENCY,
  ERR_UNSUPPORTED_KEYWORD,

  /* AIGER */
  ERR_AIGER_HEADER,
  ERR_AIGER_LATCH_DECLARATION,
  ERR_AIGER_AND_DECLARATION,

  /* BLIF */
  ERR_BLIF_LATCH_FORMAT,

  /* VERILOG */
  ERR_VERILOG_MODULE_HEADER,
  ERR_VERILOG_INPUT_DECLARATION,
  ERR_VERILOG_OUTPUT_DECLARATION,
  ERR_VERILOG_WIRE_DECLARATION,
  ERR_VERILOG_ASSIGNMENT,
  ERR_VERILOG_MODULE_INSTANTIATION_STATEMENT,
  ERR_VERILOG_MODULE_INSTANTIATION_UNDECLARED_MODULE,
  ERR_VERILOG_MODULE_INSTANTIATION_UNDECLARED_PIN,
  ERR_VERILOG_ASSIGNMENT_RHS,

  /* DIMACS */
  ERR_DIMACS_MISSING_SPEC,

  /* GENLIB */
  ERR_GENLIB_UNEXPECTED_STRUCTURE,
  ERR_GENLIB_GATE,
  ERR_GENLIB_EXPRESSION,
  ERR_GENLIB_PIN,
  ERR_GENLIB_PIN_PHASE,
  ERR_GENLIB_FAILED,

  /* SUPER */
  ERR_SUPER_INFO,
  ERR_SUPER_UNEXPECTED_STRUCTURE,
  ERR_SUPER_GATE,

  /* sentinel element to mark the end */
  NUM_STATIC_ERROR_IDS,
};


/*! pre-defined static diagnostics */
static std::pair<diagnostic_level, std::string> const diag_info[] = {
  { diagnostic_level::fatal,   "could not open file `{}`" },
  { diagnostic_level::fatal,   "could not parse line `{}`" },
  { diagnostic_level::warning, "unresolved dependencies: `{}` requires `{}`" },
  { diagnostic_level::fatal,   "unsupported keyword `{}`" },

  /* AIGER */
  { diagnostic_level::fatal, "could not parse AIGER header `{}`" },
  { diagnostic_level::fatal, "could not parse declaration of LATCH `{}`" },
  { diagnostic_level::fatal, "could not parse declaration of AND gate `{}`" },

  /* BLIF */
  { diagnostic_level::fatal, "latch format not supported `{}`" },

  /* VERILOG */
  { diagnostic_level::fatal, "cannot parse module header" },
  { diagnostic_level::fatal, "cannot parse input declaration" },
  { diagnostic_level::fatal, "cannot parse output declaration" },
  { diagnostic_level::fatal, "cannot parse wire declaration" },
  { diagnostic_level::fatal, "cannot parse assign statement" },
  { diagnostic_level::fatal, "cannot parse module instantiation statement" },
  { diagnostic_level::fatal, "cannot instantiate module: module `{}` has not been declared (ensure that the module has been parsed before)" },
  { diagnostic_level::fatal, "cannot instantiate module: pin `{}` has not been declared in module `{}`" },
  { diagnostic_level::fatal, "cannot parse expression on right-hand side of assign `{}`" },

  /* DIMACS */
  { diagnostic_level::fatal, "missing problem specification line" },

  /* GENLIB */
  { diagnostic_level::fatal, "line `{}` has unexpected structure (expected `GATE <name> <area> <expression>;`)`" },
  { diagnostic_level::fatal, "line `{}` does not start with keyword `GATE`" },
  { diagnostic_level::fatal, "expression `{}` is not immediately terminated with `;`" },
  { diagnostic_level::fatal, "unexpected `{}` token (expected `PIN`)" },
  { diagnostic_level::fatal, "unknown PIN phase type `{}` (expected `INV`, `NONINV`, or `UNKNOWN`)" },
  { diagnostic_level::fatal, "parsing failed at token `{}`" },

  /* SUPER */
  { diagnostic_level::fatal, "line `{}` has unexpected structure (expected only one data)" },
  { diagnostic_level::fatal, "line `{}` has a wrong structure (too few fields or exceeding the limit)" },
  { diagnostic_level::fatal, "gate `{}` has a wrong number of fanin (zero or exceeding the limit)" },
};
