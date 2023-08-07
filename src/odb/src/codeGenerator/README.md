# Automatic Code Generator

This is an automatic code generation tool for OpenDB objects and Iterators. To test the tool you can use the following command

``` shell
python3 gen.py --json schema.json --src_dir ../db --include_dir ../../include/odb --templates templates
```

Where schema.json is the json file that includes the requirements, src is the source files directory, include is the include directory, and templates is the directory including the jinja templates for the classes.

Empty sections are removed by default from the output.  If you need to add someting to a section that is currently empty, you can run the generator with --keep_empty to preserve them.  Once the section is filled in, the flag can be dropped and the code regnerated to remove the remaining empty sections.
