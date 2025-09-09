# Tcl Format

The most important part to take note of are:
`sta::define_cmd_args` - which defines what is printed when
a user types `help command` in the OR shell; as well as
`sta::parse_key_args` - which defines what keys/flags the
command actually parses.

Let us use `check_antennas` command for an example.

## Specification

```tcl
sta::define_cmd_args "check_antennas" { [-verbose]\
                                          [-net net]}

proc check_antennas { args } {
  sta::parse_key_args "check_antennas" args \
    keys {-report_file -net} \
    flags {-verbose -report_violating_nets}
...
}
```

## Do not compile

If you add `;# checker off` behind the command's 
`sta::define_cmd_args {} ` and `sta::parse_key_args {}` 
the function will not be compiled in the Manpages and
included in the doctests.

```tcl
sta::define_cmd_args "check_antennas" { [-verbose]\
                                          [-net net]} ;# checker off

proc check_antennas { args } {
  sta::parse_key_args "check_antennas" args \
    keys {-report_file -net} \
    flags {-verbose -report_violating_nets};# checker off
...
}
```