# DFT: Design for Testing

This tool is a implementation of Design For Testing for OpenROAD. Design for
Testing is used to add new nets and combinational logic to allow IC designs to
be tested for errors coming from physical imperfections in the tapeout.

A simple DFT insertion consist of the following parts:

* A scan_in pin where the test patterns are shifted in.
* A scan_out pin where the test patterns are read from.
* Scan cells that replace your flops with registers that allow us for testing.
* One or more scan chains (shift registers created from your scan cells).
* A scan_enable pin to allow your design to enter and leave the test mode.


# Supported Features

TODO


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
