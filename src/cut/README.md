# Logic cut

This package extracts a cloud of logic using the OpenSTA timing engine, and
passes it to ABC either through `blif` interface, or by directly constructing a
network. The ABC output is read back by a `blif` reader which is integrated to
OpenDB. `blif` writer and reader also support constants from and to OpenDB.
Alternatively, the logic can be reconstructed in OpenDB directly from ABC
structures.

## Commands

This module is not meant to be used directly by users, and as such does not provide any commands.

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

## Authors

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
