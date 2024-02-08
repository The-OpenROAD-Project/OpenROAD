# Manpages Test Framework

There are 4 regression tests we conduct for the manpages, namely:
- Translator
- Count output files
- Man functionality check
- Man-to-Tcl check

There are also 2 CI tests as part of the
They can be found in `.github/workflows/github-actions-docs-test.yml` file.

## Regression tests 

### Translator

The code file can be found [here](translator.py). 
The objective of this test is to test if the underlying `README.md`
file can be converted to roff format via regex extraction. Namely,
the script checks for equality in the number of function names,
descriptors, synopsis, options and arguments detected per Tcl command.

### Count output files

The code file can be found [here](count_outfiles.py). 
The objective of this test is to count if the expected number of files
in `man2` and `man3` are as expected. 

### Man functionality check

The code file can be found [here](man_func.tcl). 
The objective of this test is to check the functionality of the Tcl
`man` command implemented within the OpenROAD binary. 
Mode 1 is where we run `man -manpath <CMD>`, and mode 2
is where we do not specify the `-manpath` argument and just run
`man <CMD>`. 

This check makes sure that the files are compiled in the correct location
and viewable by the `man` command.

### Man-to-Tcl check

The code file can be found [here](man_tcl_check.py). 
The objective of this test is to ensure that there are similar counts of 
command in the following: `proc`, `help`, `man`. 

`proc` and `help` commands are parsed from the Tcl file, whereas
`man` commands are parsed from the README file. 

## CI tests

These two tests ensure that the documents and top-level Tcl files 
are formatted properly to catch manpage extraction or compilation errors. 

- Tcl Syntax Parser [code](man_tcl_params.py) 
- Readme Syntax Parser [code](readme_parser.py)

## New Test Checklist 

Adding a new test called `func`, you must create/update the following:
- `func.py|tcl`: Test script.
- `func.ok`: Log output of the test.
- `.*ok`: Ideal file output of the test (can be `def`, `lef` etc).
- `regression_tests.tcl`: Update the name of the test. In this case, `func`.

## Authors

Jack Luar (Advisor: Cho Moon)

## License

BSD 3-Clause License. See [LICENSE](../../../LICENSE) file.