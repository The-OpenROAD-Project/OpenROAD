# DFT: Design for Testing

This tool is an implementation of Design For Testing. New nets and logic are added to allow IC designs to
be tested for errors in manufacturing.   Physical imperfections can cause hard failures and variability can cause timing errors.

A simple DFT insertion consist of the following parts:

* A scan_in pin where the test patterns are shifted in.
* A scan_out pin where the test patterns are read from.
* Scan cells that replace flops with registers that allow for testing.
* One or more scan chains (shift registers created from your scan cells).
* A scan_enable pin to allow your design to enter and leave the test mode.


# Supported Features

TODO


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
