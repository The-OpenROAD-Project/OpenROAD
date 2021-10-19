%typemap(out) odb::defout::Version, defout::Version
{
  Tcl_Obj *obj;
  if ($1 == odb::defout::Version::DEF_5_3) {
    obj = Tcl_NewStringObj("DEF_5_3", -1);
  } else if ($1 == odb::defout::Version::DEF_5_4) {
    obj = Tcl_NewStringObj("DEF_5_4", -1);
  } else if ($1 == odb::defout::Version::DEF_5_5) {
    obj = Tcl_NewStringObj("DEF_5_5", -1);
  } else if ($1 == odb::defout::Version::DEF_5_6) {
    obj = Tcl_NewStringObj("DEF_5_6", -1);
  } else if ($1 == odb::defout::Version::DEF_5_8) {
    obj = Tcl_NewStringObj("DEF_5_8", -1);
  } else {
    return TCL_ERROR;
  }
  Tcl_SetObjResult(interp, obj);
}

%typemap(in) odb::defout::Version, defout::Version
{
  char *str = Tcl_GetStringFromObj($input, 0);
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
  } else {
    return TCL_ERROR;
  }
}

%typemap(typecheck) odb::defout::Version, defout::Version
{
  char *str = Tcl_GetStringFromObj($input, 0);
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
