# Automatic Code Generator

This is an automatic code generation tool for OpenDB objects and Iterators. To test the tool you can use the following command

``` shell
python3 gen.py --json schema.json --src_dir ../db --include_dir ../../include/odb --impl impl
```

Where schema.json is the json file that includes the requirements, src is the source files directory, include is the include directory, and impl is the directory including the jinja templates for the classes.
