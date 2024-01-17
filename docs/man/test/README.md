# Doc test framework

There are `n` tests, documentation TBC.

## Test Description

TODO

## Checklist 
Adding a new test called `func`, you must create/update the following:
- `func.py|tcl`: Test script.
- `func.ok`: Log output of the test.
- `.*ok`: Ideal file output of the test (can be `def`, `lef` etc).
- `regression_tests.tcl`: Update the name of the test. In this case, `func`.

```
# Please replace with your path to OR.
cd ~/OpenROAD/docs/manpages

# if you installed OR via prebuilt binaries, you might need to symlink as the following script only works with OR in a particular directory.
# mkdir -p ~/OpenROAD/build/src && cd ~/OpenROAD/build/src
# ln -s /usr/bin/openroad .

# run the tests
./test/regression 
```

## TODO
- This documentation
- MD to ROFF translation tests: options, synopsis, description
- Tcl test for `man` command
- Missing manpage test: Help exists but no manpage. 
- Check consistency of options between `help` and `man`. The command line options should MATCH.