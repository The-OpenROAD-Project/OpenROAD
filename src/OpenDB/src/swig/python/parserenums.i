%typemap(out) odb::defout::Version, defout::Version
{
  PyObject *obj;
  if ($1 == odb::defout::Version::DEF_5_3) {
    obj = PyString_FromString("DEF_5_3");
  } else if ($1 == odb::defout::Version::DEF_5_4) {
    obj = PyString_FromString("DEF_5_4");
  } else if ($1 == odb::defout::Version::DEF_5_5) {
    obj = PyString_FromString("DEF_5_5");
  } else if ($1 == odb::defout::Version::DEF_5_6) {
    obj = PyString_FromString("DEF_5_6");
  } else if ($1 == odb::defout::Version::DEF_5_8) {
    obj = PyString_FromString("DEF_5_8");
  }
  $result = obj;
}

%typemap(in) odb::defout::Version, defout::Version
{
  char *str = PyString_AsString($input);
  if (strcasecmp(str, "DEF_5_3") == 0 || strcasecmp(str, "5.3") == 0) {
    $1 = odb::defout::Version::DEF_5_3;
  } else if (strcasecmp(str, "DEF_5_4") == 0 || strcasecmp(str, "5.4") == 0) {
    $1 = odb::defout::Version::DEF_5_4;
  } else if (strcasecmp(str, "DEF_5_5") == 0 || strcasecmp(str, "5.5") == 0) {
    $1 = odb::defout::Version::DEF_5_5;
  } else if (strcasecmp(str, "DEF_5_6") == 0 || strcasecmp(str, "5.6") == 0) {
    $1 = odb::defout::Version::DEF_5_6;
  } else if (strcasecmp(str, "DEF_5_8") == 0 || strcasecmp(str, "5.8") == 0) {
    $1 = odb::defout::Version::DEF_5_8;
  }
}

%typemap(typecheck) odb::defout::Version, defout::Version
{
  char *str = PyString_AsString($input);
  bool found = false;
  if (str) {
    if (strcasecmp(str, "DEF_5_3") == 0 || strcasecmp(str, "5.3") == 0) {
      found = true;
    } else if (strcasecmp(str, "DEF_5_4") == 0 || strcasecmp(str, "5.4") == 0) {
      found = true;
    } else if (strcasecmp(str, "DEF_5_5") == 0 || strcasecmp(str, "5.5") == 0) {
      found = true;
    } else if (strcasecmp(str, "DEF_5_6") == 0 || strcasecmp(str, "5.6") == 0) {
      found = true;
    } else if (strcasecmp(str, "DEF_5_8") == 0 || strcasecmp(str, "5.8") == 0) {
      found = true;
    }
  }
  if (found) {
    $1 = 1;
  } else {
    $1 = 0;
  }
}
