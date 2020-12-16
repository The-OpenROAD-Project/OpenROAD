
%typemap(out) odb::dbOrientType, dbOrientType {
	PyObject *obj;
	if ($1.getValue() == odb::dbOrientType::Value::R0) {
		obj = PyString_FromString("R0");
	} else if ($1.getValue() == odb::dbOrientType::Value::R90) {
		obj = PyString_FromString("R90");
	} else if ($1.getValue() == odb::dbOrientType::Value::R180) {
		obj = PyString_FromString("R180");
	} else if ($1.getValue() == odb::dbOrientType::Value::R270) {
		obj = PyString_FromString("R270");
	} else if ($1.getValue() == odb::dbOrientType::Value::MY) {
		obj = PyString_FromString("MY");
	} else if ($1.getValue() == odb::dbOrientType::Value::MYR90) {
		obj = PyString_FromString("MYR90");
	} else if ($1.getValue() == odb::dbOrientType::Value::MX) {
		obj = PyString_FromString("MX");
	} else if ($1.getValue() == odb::dbOrientType::Value::MXR90) {
		obj = PyString_FromString("MXR90");
	}
	$result=obj;
}
%typemap(in) odb::dbOrientType, dbOrientType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "R0") == 0) {
		$1 = odb::dbOrientType::Value::R0;
	} else if (strcasecmp(str, "R90") == 0) {
		$1 = odb::dbOrientType::Value::R90;
	} else if (strcasecmp(str, "R180") == 0) {
		$1 = odb::dbOrientType::Value::R180;
	} else if (strcasecmp(str, "R270") == 0) {
		$1 = odb::dbOrientType::Value::R270;
	} else if (strcasecmp(str, "MY") == 0) {
		$1 = odb::dbOrientType::Value::MY;
	} else if (strcasecmp(str, "MYR90") == 0) {
		$1 = odb::dbOrientType::Value::MYR90;
	} else if (strcasecmp(str, "MX") == 0) {
		$1 = odb::dbOrientType::Value::MX;
	} else if (strcasecmp(str, "MXR90") == 0) {
		$1 = odb::dbOrientType::Value::MXR90;
	}
}
%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) odb::dbOrientType, dbOrientType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "R0") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "R90") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "R180") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "R270") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MY") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MYR90") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MX") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MXR90") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbSigType, dbSigType {
	PyObject *obj;
	if ($1.getValue() == odb::dbSigType::Value::SIGNAL) {
		obj = PyString_FromString("SIGNAL");
	} else if ($1.getValue() == odb::dbSigType::Value::POWER) {
		obj = PyString_FromString("POWER");
	} else if ($1.getValue() == odb::dbSigType::Value::GROUND) {
		obj = PyString_FromString("GROUND");
	} else if ($1.getValue() == odb::dbSigType::Value::CLOCK) {
		obj = PyString_FromString("CLOCK");
	} else if ($1.getValue() == odb::dbSigType::Value::ANALOG) {
		obj = PyString_FromString("ANALOG");
	} else if ($1.getValue() == odb::dbSigType::Value::RESET) {
		obj = PyString_FromString("RESET");
	} else if ($1.getValue() == odb::dbSigType::Value::SCAN) {
		obj = PyString_FromString("SCAN");
	} else if ($1.getValue() == odb::dbSigType::Value::TIEOFF) {
		obj = PyString_FromString("TIEOFF");
	}
	$result=obj;
}
%typemap(in) odb::dbSigType, dbSigType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "SIGNAL") == 0) {
		$1 = odb::dbSigType::Value::SIGNAL;
	} else if (strcasecmp(str, "POWER") == 0) {
		$1 = odb::dbSigType::Value::POWER;
	} else if (strcasecmp(str, "GROUND") == 0) {
		$1 = odb::dbSigType::Value::GROUND;
	} else if (strcasecmp(str, "CLOCK") == 0) {
		$1 = odb::dbSigType::Value::CLOCK;
	} else if (strcasecmp(str, "ANALOG") == 0) {
		$1 = odb::dbSigType::Value::ANALOG;
	} else if (strcasecmp(str, "RESET") == 0) {
		$1 = odb::dbSigType::Value::RESET;
	} else if (strcasecmp(str, "SCAN") == 0) {
		$1 = odb::dbSigType::Value::SCAN;
	} else if (strcasecmp(str, "TIEOFF") == 0) {
		$1 = odb::dbSigType::Value::TIEOFF;
	}
}
%typemap(typecheck) odb::dbSigType, dbSigType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "SIGNAL") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "POWER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "GROUND") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CLOCK") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ANALOG") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "RESET") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SCAN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "TIEOFF") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbIoType, dbIoType {
	PyObject *obj;
	if ($1.getValue() == odb::dbIoType::Value::INPUT) {
		obj = PyString_FromString("INPUT");
	} else if ($1.getValue() == odb::dbIoType::Value::OUTPUT) {
		obj = PyString_FromString("OUTPUT");
	} else if ($1.getValue() == odb::dbIoType::Value::INOUT) {
		obj = PyString_FromString("INOUT");
	} else if ($1.getValue() == odb::dbIoType::Value::FEEDTHRU) {
		obj = PyString_FromString("FEEDTHRU");
	}
	$result=obj;
}
%typemap(in) odb::dbIoType, dbIoType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "INPUT") == 0) {
		$1 = odb::dbIoType::Value::INPUT;
	} else if (strcasecmp(str, "OUTPUT") == 0) {
		$1 = odb::dbIoType::Value::OUTPUT;
	} else if (strcasecmp(str, "INOUT") == 0) {
		$1 = odb::dbIoType::Value::INOUT;
	} else if (strcasecmp(str, "FEEDTHRU") == 0) {
		$1 = odb::dbIoType::Value::FEEDTHRU;
	}
}
%typemap(typecheck) odb::dbIoType, dbIoType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "INPUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "OUTPUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "INOUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "FEEDTHRU") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbPlacementStatus, dbPlacementStatus {
	PyObject *obj;
	if ($1.getValue() == odb::dbPlacementStatus::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::UNPLACED) {
		obj = PyString_FromString("UNPLACED");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::SUGGESTED) {
		obj = PyString_FromString("SUGGESTED");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::PLACED) {
		obj = PyString_FromString("PLACED");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::LOCKED) {
		obj = PyString_FromString("LOCKED");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::FIRM) {
		obj = PyString_FromString("FIRM");
	} else if ($1.getValue() == odb::dbPlacementStatus::Value::COVER) {
		obj = PyString_FromString("COVER");
	}
	$result=obj;
}
%typemap(in) odb::dbPlacementStatus, dbPlacementStatus {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbPlacementStatus::Value::NONE;
	} else if (strcasecmp(str, "UNPLACED") == 0) {
		$1 = odb::dbPlacementStatus::Value::UNPLACED;
	} else if (strcasecmp(str, "SUGGESTED") == 0) {
		$1 = odb::dbPlacementStatus::Value::SUGGESTED;
	} else if (strcasecmp(str, "PLACED") == 0) {
		$1 = odb::dbPlacementStatus::Value::PLACED;
	} else if (strcasecmp(str, "LOCKED") == 0) {
		$1 = odb::dbPlacementStatus::Value::LOCKED;
	} else if (strcasecmp(str, "FIRM") == 0) {
		$1 = odb::dbPlacementStatus::Value::FIRM;
	} else if (strcasecmp(str, "COVER") == 0) {
		$1 = odb::dbPlacementStatus::Value::COVER;
	}
}
%typemap(typecheck) odb::dbPlacementStatus, dbPlacementStatus {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "UNPLACED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SUGGESTED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PLACED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "LOCKED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "FIRM") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "COVER") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbMasterType, dbMasterType {
	PyObject *obj;
	if ($1.getValue() == odb::dbMasterType::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbMasterType::Value::COVER) {
		obj = PyString_FromString("COVER");
	} else if ($1.getValue() == odb::dbMasterType::Value::COVER_BUMP) {
		obj = PyString_FromString("COVER_BUMP");
	} else if ($1.getValue() == odb::dbMasterType::Value::RING) {
		obj = PyString_FromString("RING");
	} else if ($1.getValue() == odb::dbMasterType::Value::BLOCK) {
		obj = PyString_FromString("BLOCK");
	} else if ($1.getValue() == odb::dbMasterType::Value::BLOCK_BLACKBOX) {
		obj = PyString_FromString("BLOCK_BLACKBOX");
	} else if ($1.getValue() == odb::dbMasterType::Value::BLOCK_SOFT) {
		obj = PyString_FromString("BLOCK_SOFT");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD) {
		obj = PyString_FromString("PAD");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_INPUT) {
		obj = PyString_FromString("PAD_INPUT");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_OUTPUT) {
		obj = PyString_FromString("PAD_OUTPUT");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_INOUT) {
		obj = PyString_FromString("PAD_INOUT");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_POWER) {
		obj = PyString_FromString("PAD_POWER");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_SPACER) {
		obj = PyString_FromString("PAD_SPACER");
	} else if ($1.getValue() == odb::dbMasterType::Value::PAD_AREAIO) {
		obj = PyString_FromString("PAD_AREAIO");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE) {
		obj = PyString_FromString("CORE");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_FEEDTHRU) {
		obj = PyString_FromString("CORE_FEEDTHRU");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_TIEHIGH) {
		obj = PyString_FromString("CORE_TIEHIGH");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_TIELOW) {
		obj = PyString_FromString("CORE_TIELOW");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_SPACER) {
		obj = PyString_FromString("CORE_SPACER");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_ANTENNACELL) {
		obj = PyString_FromString("CORE_ANTENNACELL");
	} else if ($1.getValue() == odb::dbMasterType::Value::CORE_WELLTAP) {
		obj = PyString_FromString("CORE_WELLTAP");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP) {
		obj = PyString_FromString("ENDCAP");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_PRE) {
		obj = PyString_FromString("ENDCAP_PRE");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_POST) {
		obj = PyString_FromString("ENDCAP_POST");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_TOPLEFT) {
		obj = PyString_FromString("ENDCAP_TOPLEFT");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_TOPRIGHT) {
		obj = PyString_FromString("ENDCAP_TOPRIGHT");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_BOTTOMLEFT) {
		obj = PyString_FromString("ENDCAP_BOTTOMLEFT");
	} else if ($1.getValue() == odb::dbMasterType::Value::ENDCAP_BOTTOMRIGHT) {
		obj = PyString_FromString("ENDCAP_BOTTOMRIGHT");
	}
	$result=obj;
}
%typemap(in) odb::dbMasterType, dbMasterType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbMasterType::Value::NONE;
	} else if (strcasecmp(str, "COVER") == 0) {
		$1 = odb::dbMasterType::Value::COVER;
	} else if (strcasecmp(str, "COVER_BUMP") == 0) {
		$1 = odb::dbMasterType::Value::COVER_BUMP;
	} else if (strcasecmp(str, "RING") == 0) {
		$1 = odb::dbMasterType::Value::RING;
	} else if (strcasecmp(str, "BLOCK") == 0) {
		$1 = odb::dbMasterType::Value::BLOCK;
	} else if (strcasecmp(str, "BLOCK_BLACKBOX") == 0) {
		$1 = odb::dbMasterType::Value::BLOCK_BLACKBOX;
	} else if (strcasecmp(str, "BLOCK_SOFT") == 0) {
		$1 = odb::dbMasterType::Value::BLOCK_SOFT;
	} else if (strcasecmp(str, "PAD") == 0) {
		$1 = odb::dbMasterType::Value::PAD;
	} else if (strcasecmp(str, "PAD_INPUT") == 0) {
		$1 = odb::dbMasterType::Value::PAD_INPUT;
	} else if (strcasecmp(str, "PAD_OUTPUT") == 0) {
		$1 = odb::dbMasterType::Value::PAD_OUTPUT;
	} else if (strcasecmp(str, "PAD_INOUT") == 0) {
		$1 = odb::dbMasterType::Value::PAD_INOUT;
	} else if (strcasecmp(str, "PAD_POWER") == 0) {
		$1 = odb::dbMasterType::Value::PAD_POWER;
	} else if (strcasecmp(str, "PAD_SPACER") == 0) {
		$1 = odb::dbMasterType::Value::PAD_SPACER;
	} else if (strcasecmp(str, "PAD_AREAIO") == 0) {
		$1 = odb::dbMasterType::Value::PAD_AREAIO;
	} else if (strcasecmp(str, "CORE") == 0) {
		$1 = odb::dbMasterType::Value::CORE;
	} else if (strcasecmp(str, "CORE_FEEDTHRU") == 0) {
		$1 = odb::dbMasterType::Value::CORE_FEEDTHRU;
	} else if (strcasecmp(str, "CORE_TIEHIGH") == 0) {
		$1 = odb::dbMasterType::Value::CORE_TIEHIGH;
	} else if (strcasecmp(str, "CORE_TIELOW") == 0) {
		$1 = odb::dbMasterType::Value::CORE_TIELOW;
	} else if (strcasecmp(str, "CORE_SPACER") == 0) {
		$1 = odb::dbMasterType::Value::CORE_SPACER;
	} else if (strcasecmp(str, "CORE_ANTENNACELL") == 0) {
		$1 = odb::dbMasterType::Value::CORE_ANTENNACELL;
	} else if (strcasecmp(str, "CORE_WELLTAP") == 0) {
		$1 = odb::dbMasterType::Value::CORE_WELLTAP;
	} else if (strcasecmp(str, "ENDCAP") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP;
	} else if (strcasecmp(str, "ENDCAP_PRE") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_PRE;
	} else if (strcasecmp(str, "ENDCAP_POST") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_POST;
	} else if (strcasecmp(str, "ENDCAP_TOPLEFT") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_TOPLEFT;
	} else if (strcasecmp(str, "ENDCAP_TOPRIGHT") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_TOPRIGHT;
	} else if (strcasecmp(str, "ENDCAP_BOTTOMLEFT") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_BOTTOMLEFT;
	} else if (strcasecmp(str, "ENDCAP_BOTTOMRIGHT") == 0) {
		$1 = odb::dbMasterType::Value::ENDCAP_BOTTOMRIGHT;
	}
}
%typemap(typecheck) odb::dbMasterType, dbMasterType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "COVER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "COVER_BUMP") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "RING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCK") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCK_BLACKBOX") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCK_SOFT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_INPUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_OUTPUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_INOUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_POWER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_SPACER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD_AREAIO") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_FEEDTHRU") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_TIEHIGH") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_TIELOW") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_SPACER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_ANTENNACELL") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE_WELLTAP") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_PRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_POST") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_TOPLEFT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_TOPRIGHT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_BOTTOMLEFT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ENDCAP_BOTTOMRIGHT") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbTechLayerType, dbTechLayerType {
	PyObject *obj;
	if ($1.getValue() == odb::dbTechLayerType::Value::ROUTING) {
		obj = PyString_FromString("ROUTING");
	} else if ($1.getValue() == odb::dbTechLayerType::Value::CUT) {
		obj = PyString_FromString("CUT");
	} else if ($1.getValue() == odb::dbTechLayerType::Value::MASTERSLICE) {
		obj = PyString_FromString("MASTERSLICE");
	} else if ($1.getValue() == odb::dbTechLayerType::Value::OVERLAP) {
		obj = PyString_FromString("OVERLAP");
	} else if ($1.getValue() == odb::dbTechLayerType::Value::IMPLANT) {
		obj = PyString_FromString("IMPLANT");
	} else if ($1.getValue() == odb::dbTechLayerType::Value::NONE) {
		obj = PyString_FromString("NONE");
	}
	$result=obj;
}
%typemap(in) odb::dbTechLayerType, dbTechLayerType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "ROUTING") == 0) {
		$1 = odb::dbTechLayerType::Value::ROUTING;
	} else if (strcasecmp(str, "CUT") == 0) {
		$1 = odb::dbTechLayerType::Value::CUT;
	} else if (strcasecmp(str, "MASTERSLICE") == 0) {
		$1 = odb::dbTechLayerType::Value::MASTERSLICE;
	} else if (strcasecmp(str, "OVERLAP") == 0) {
		$1 = odb::dbTechLayerType::Value::OVERLAP;
	} else if (strcasecmp(str, "IMPLANT") == 0) {
		$1 = odb::dbTechLayerType::Value::IMPLANT;
	} else if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbTechLayerType::Value::NONE;
	}
}
%typemap(typecheck) odb::dbTechLayerType, dbTechLayerType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "ROUTING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CUT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MASTERSLICE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "OVERLAP") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "IMPLANT") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "NONE") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbTechLayerDir, dbTechLayerDir {
	PyObject *obj;
	if ($1.getValue() == odb::dbTechLayerDir::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbTechLayerDir::Value::HORIZONTAL) {
		obj = PyString_FromString("HORIZONTAL");
	} else if ($1.getValue() == odb::dbTechLayerDir::Value::VERTICAL) {
		obj = PyString_FromString("VERTICAL");
	}
	$result=obj;
}
%typemap(in) odb::dbTechLayerDir, dbTechLayerDir {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbTechLayerDir::Value::NONE;
	} else if (strcasecmp(str, "HORIZONTAL") == 0) {
		$1 = odb::dbTechLayerDir::Value::HORIZONTAL;
	} else if (strcasecmp(str, "VERTICAL") == 0) {
		$1 = odb::dbTechLayerDir::Value::VERTICAL;
	}
}
%typemap(typecheck) odb::dbTechLayerDir, dbTechLayerDir {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "HORIZONTAL") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "VERTICAL") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbRowDir, dbRowDir {
	PyObject *obj;
	if ($1.getValue() == odb::dbRowDir::Value::HORIZONTAL) {
		obj = PyString_FromString("HORIZONTAL");
	} else if ($1.getValue() == odb::dbRowDir::Value::VERTICAL) {
		obj = PyString_FromString("VERTICAL");
	}
	$result=obj;
}
%typemap(in) odb::dbRowDir, dbRowDir {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "HORIZONTAL") == 0) {
		$1 = odb::dbRowDir::Value::HORIZONTAL;
	} else if (strcasecmp(str, "VERTICAL") == 0) {
		$1 = odb::dbRowDir::Value::VERTICAL;
	}
}
%typemap(typecheck) odb::dbRowDir, dbRowDir {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "HORIZONTAL") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "VERTICAL") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbBoxOwner, dbBoxOwner {
	PyObject *obj;
	if ($1.getValue() == odb::dbBoxOwner::Value::UNKNOWN) {
		obj = PyString_FromString("UNKNOWN");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::BLOCK) {
		obj = PyString_FromString("BLOCK");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::INST) {
		obj = PyString_FromString("INST");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::BTERM) {
		obj = PyString_FromString("BTERM");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::VIA) {
		obj = PyString_FromString("VIA");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::OBSTRUCTION) {
		obj = PyString_FromString("OBSTRUCTION");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::SWIRE) {
		obj = PyString_FromString("SWIRE");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::BLOCKAGE) {
		obj = PyString_FromString("BLOCKAGE");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::MASTER) {
		obj = PyString_FromString("MASTER");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::MPIN) {
		obj = PyString_FromString("MPIN");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::TECH_VIA) {
		obj = PyString_FromString("TECH_VIA");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::REGION) {
		obj = PyString_FromString("REGION");
	} else if ($1.getValue() == odb::dbBoxOwner::Value::BPIN) {
		obj = PyString_FromString("BPIN");
	}
	$result=obj;
}
%typemap(in) odb::dbBoxOwner, dbBoxOwner {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "UNKNOWN") == 0) {
		$1 = odb::dbBoxOwner::Value::UNKNOWN;
	} else if (strcasecmp(str, "BLOCK") == 0) {
		$1 = odb::dbBoxOwner::Value::BLOCK;
	} else if (strcasecmp(str, "INST") == 0) {
		$1 = odb::dbBoxOwner::Value::INST;
	} else if (strcasecmp(str, "BTERM") == 0) {
		$1 = odb::dbBoxOwner::Value::BTERM;
	} else if (strcasecmp(str, "VIA") == 0) {
		$1 = odb::dbBoxOwner::Value::VIA;
	} else if (strcasecmp(str, "OBSTRUCTION") == 0) {
		$1 = odb::dbBoxOwner::Value::OBSTRUCTION;
	} else if (strcasecmp(str, "SWIRE") == 0) {
		$1 = odb::dbBoxOwner::Value::SWIRE;
	} else if (strcasecmp(str, "BLOCKAGE") == 0) {
		$1 = odb::dbBoxOwner::Value::BLOCKAGE;
	} else if (strcasecmp(str, "MASTER") == 0) {
		$1 = odb::dbBoxOwner::Value::MASTER;
	} else if (strcasecmp(str, "MPIN") == 0) {
		$1 = odb::dbBoxOwner::Value::MPIN;
	} else if (strcasecmp(str, "TECH_VIA") == 0) {
		$1 = odb::dbBoxOwner::Value::TECH_VIA;
	} else if (strcasecmp(str, "REGION") == 0) {
		$1 = odb::dbBoxOwner::Value::REGION;
	} else if (strcasecmp(str, "BPIN") == 0) {
		$1 = odb::dbBoxOwner::Value::BPIN;
	}
}
%typemap(typecheck) odb::dbBoxOwner, dbBoxOwner {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "UNKNOWN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCK") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "INST") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BTERM") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "VIA") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "OBSTRUCTION") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCKAGE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MASTER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MPIN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "TECH_VIA") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "REGION") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BPIN") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbPolygonOwner, dbPolygonOwner {
	PyObject *obj;
	if ($1.getValue() == odb::dbPolygonOwner::Value::UNKNOWN) {
		obj = PyString_FromString("UNKNOWN");
	} else if ($1.getValue() == odb::dbPolygonOwner::Value::BPIN) {
		obj = PyString_FromString("BPIN");
	} else if ($1.getValue() == odb::dbPolygonOwner::Value::OBSTRUCTION) {
		obj = PyString_FromString("OBSTRUCTION");
	} else if ($1.getValue() == odb::dbPolygonOwner::Value::SWIRE) {
		obj = PyString_FromString("SWIRE");
	}
	$result=obj;
}
%typemap(in) odb::dbPolygonOwner, dbPolygonOwner {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "UNKNOWN") == 0) {
		$1 = odb::dbPolygonOwner::Value::UNKNOWN;
	} else if (strcasecmp(str, "BPIN") == 0) {
		$1 = odb::dbPolygonOwner::Value::BPIN;
	} else if (strcasecmp(str, "OBSTRUCTION") == 0) {
		$1 = odb::dbPolygonOwner::Value::OBSTRUCTION;
	} else if (strcasecmp(str, "SWIRE") == 0) {
		$1 = odb::dbPolygonOwner::Value::SWIRE;
	}
}
%typemap(typecheck) odb::dbPolygonOwner, dbPolygonOwner {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "UNKNOWN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BPIN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "OBSTRUCTION") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SWIRE") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbWireType, dbWireType {
	PyObject *obj;
	if ($1.getValue() == odb::dbWireType::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbWireType::Value::COVER) {
		obj = PyString_FromString("COVER");
	} else if ($1.getValue() == odb::dbWireType::Value::FIXED) {
		obj = PyString_FromString("FIXED");
	} else if ($1.getValue() == odb::dbWireType::Value::ROUTED) {
		obj = PyString_FromString("ROUTED");
	} else if ($1.getValue() == odb::dbWireType::Value::SHIELD) {
		obj = PyString_FromString("SHIELD");
	} else if ($1.getValue() == odb::dbWireType::Value::NOSHIELD) {
		obj = PyString_FromString("NOSHIELD");
	}
	$result=obj;
}
%typemap(in) odb::dbWireType, dbWireType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbWireType::Value::NONE;
	} else if (strcasecmp(str, "COVER") == 0) {
		$1 = odb::dbWireType::Value::COVER;
	} else if (strcasecmp(str, "FIXED") == 0) {
		$1 = odb::dbWireType::Value::FIXED;
	} else if (strcasecmp(str, "ROUTED") == 0) {
		$1 = odb::dbWireType::Value::ROUTED;
	} else if (strcasecmp(str, "SHIELD") == 0) {
		$1 = odb::dbWireType::Value::SHIELD;
	} else if (strcasecmp(str, "NOSHIELD") == 0) {
		$1 = odb::dbWireType::Value::NOSHIELD;
	}
}
%typemap(typecheck,precedence=SWIG_TYPECHECK_POINTER) odb::dbWireType, dbWireType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "COVER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "FIXED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ROUTED") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SHIELD") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "NOSHIELD") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbWireShapeType, dbWireShapeType {
	PyObject *obj;
	if ($1.getValue() == odb::dbWireShapeType::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::RING) {
		obj = PyString_FromString("RING");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::PADRING) {
		obj = PyString_FromString("PADRING");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::BLOCKRING) {
		obj = PyString_FromString("BLOCKRING");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::STRIPE) {
		obj = PyString_FromString("STRIPE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::FOLLOWPIN) {
		obj = PyString_FromString("FOLLOWPIN");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::IOWIRE) {
		obj = PyString_FromString("IOWIRE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::COREWIRE) {
		obj = PyString_FromString("COREWIRE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::BLOCKWIRE) {
		obj = PyString_FromString("BLOCKWIRE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::BLOCKAGEWIRE) {
		obj = PyString_FromString("BLOCKAGEWIRE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::FILLWIRE) {
		obj = PyString_FromString("FILLWIRE");
	} else if ($1.getValue() == odb::dbWireShapeType::Value::DRCFILL) {
		obj = PyString_FromString("DRCFILL");
	}
	$result=obj;
}
%typemap(in) odb::dbWireShapeType, dbWireShapeType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbWireShapeType::Value::NONE;
	} else if (strcasecmp(str, "RING") == 0) {
		$1 = odb::dbWireShapeType::Value::RING;
	} else if (strcasecmp(str, "PADRING") == 0) {
		$1 = odb::dbWireShapeType::Value::PADRING;
	} else if (strcasecmp(str, "BLOCKRING") == 0) {
		$1 = odb::dbWireShapeType::Value::BLOCKRING;
	} else if (strcasecmp(str, "STRIPE") == 0) {
		$1 = odb::dbWireShapeType::Value::STRIPE;
	} else if (strcasecmp(str, "FOLLOWPIN") == 0) {
		$1 = odb::dbWireShapeType::Value::FOLLOWPIN;
	} else if (strcasecmp(str, "IOWIRE") == 0) {
		$1 = odb::dbWireShapeType::Value::IOWIRE;
	} else if (strcasecmp(str, "COREWIRE") == 0) {
		$1 = odb::dbWireShapeType::Value::COREWIRE;
	} else if (strcasecmp(str, "BLOCKWIRE") == 0) {
		$1 = odb::dbWireShapeType::Value::BLOCKWIRE;
	} else if (strcasecmp(str, "BLOCKAGEWIRE") == 0) {
		$1 = odb::dbWireShapeType::Value::BLOCKAGEWIRE;
	} else if (strcasecmp(str, "FILLWIRE") == 0) {
		$1 = odb::dbWireShapeType::Value::FILLWIRE;
	} else if (strcasecmp(str, "DRCFILL") == 0) {
		$1 = odb::dbWireShapeType::Value::DRCFILL;
	}
}
%typemap(typecheck) odb::dbWireShapeType, dbWireShapeType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "RING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PADRING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCKRING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "STRIPE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "FOLLOWPIN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "IOWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "COREWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCKWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "BLOCKAGEWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "FILLWIRE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "DRCFILL") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbSiteClass, dbSiteClass {
	PyObject *obj;
	if ($1.getValue() == odb::dbSiteClass::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbSiteClass::Value::PAD) {
		obj = PyString_FromString("PAD");
	} else if ($1.getValue() == odb::dbSiteClass::Value::CORE) {
		obj = PyString_FromString("CORE");
	}
	$result=obj;
}
%typemap(in) odb::dbSiteClass, dbSiteClass {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbSiteClass::Value::NONE;
	} else if (strcasecmp(str, "PAD") == 0) {
		$1 = odb::dbSiteClass::Value::PAD;
	} else if (strcasecmp(str, "CORE") == 0) {
		$1 = odb::dbSiteClass::Value::CORE;
	}
}
%typemap(typecheck) odb::dbSiteClass, dbSiteClass {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "PAD") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "CORE") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbOnOffType, dbOnOffType {
	PyObject *obj;
	if ($1.getValue() == odb::dbOnOffType::Value::OFF) {
		obj = PyString_FromString("OFF");
	} else if ($1.getValue() == odb::dbOnOffType::Value::ON) {
		obj = PyString_FromString("ON");
	}
	$result=obj;
}
%typemap(in) odb::dbOnOffType, dbOnOffType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "OFF") == 0) {
		$1 = odb::dbOnOffType::Value::OFF;
	} else if (strcasecmp(str, "ON") == 0) {
		$1 = odb::dbOnOffType::Value::ON;
	}
}
%typemap(typecheck) odb::dbOnOffType, dbOnOffType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "OFF") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ON") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbClMeasureType, dbClMeasureType {
	PyObject *obj;
	if ($1.getValue() == odb::dbClMeasureType::Value::EUCLIDEAN) {
		obj = PyString_FromString("EUCLIDEAN");
	} else if ($1.getValue() == odb::dbClMeasureType::Value::MAXXY) {
		obj = PyString_FromString("MAXXY");
	}
	$result=obj;
}
%typemap(in) odb::dbClMeasureType, dbClMeasureType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "EUCLIDEAN") == 0) {
		$1 = odb::dbClMeasureType::Value::EUCLIDEAN;
	} else if (strcasecmp(str, "MAXXY") == 0) {
		$1 = odb::dbClMeasureType::Value::MAXXY;
	}
}
%typemap(typecheck) odb::dbClMeasureType, dbClMeasureType {
	char *str = PyString_AsString($input);
	bool found = false;
	if (str) {
		if (strcasecmp(str, "EUCLIDEAN") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "MAXXY") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbJournalEntryType, dbJournalEntryType {
	PyObject *obj;
	if ($1.getValue() == odb::dbJournalEntryType::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbJournalEntryType::Value::OWNER) {
		obj = PyString_FromString("OWNER");
	} else if ($1.getValue() == odb::dbJournalEntryType::Value::ADD) {
		obj = PyString_FromString("ADD");
	} else if ($1.getValue() == odb::dbJournalEntryType::Value::DESTROY) {
		obj = PyString_FromString("DESTROY");
	}
	$result=obj;
}
%typemap(in) odb::dbJournalEntryType, dbJournalEntryType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbJournalEntryType::Value::NONE;
	} else if (strcasecmp(str, "OWNER") == 0) {
		$1 = odb::dbJournalEntryType::Value::OWNER;
	} else if (strcasecmp(str, "ADD") == 0) {
		$1 = odb::dbJournalEntryType::Value::ADD;
	} else if (strcasecmp(str, "DESTROY") == 0) {
		$1 = odb::dbJournalEntryType::Value::DESTROY;
	}
}
%typemap(typecheck) odb::dbJournalEntryType, dbJournalEntryType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "OWNER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "ADD") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "DESTROY") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbDirection, dbDirection {
	PyObject *obj;
	if ($1.getValue() == odb::dbDirection::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbDirection::Value::NORTH) {
		obj = PyString_FromString("NORTH");
	} else if ($1.getValue() == odb::dbDirection::Value::EAST) {
		obj = PyString_FromString("EAST");
	} else if ($1.getValue() == odb::dbDirection::Value::SOUTH) {
		obj = PyString_FromString("SOUTH");
	} else if ($1.getValue() == odb::dbDirection::Value::WEST) {
		obj = PyString_FromString("WEST");
	}
	$result=obj;
}
%typemap(in) odb::dbDirection, dbDirection {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbDirection::Value::NONE;
	} else if (strcasecmp(str, "NORTH") == 0) {
		$1 = odb::dbDirection::Value::NORTH;
	} else if (strcasecmp(str, "EAST") == 0) {
		$1 = odb::dbDirection::Value::EAST;
	} else if (strcasecmp(str, "SOUTH") == 0) {
		$1 = odb::dbDirection::Value::SOUTH;
	} else if (strcasecmp(str, "WEST") == 0) {
		$1 = odb::dbDirection::Value::WEST;
	}
}
%typemap(typecheck) odb::dbDirection, dbDirection {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "NORTH") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "EAST") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SOUTH") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "WEST") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbRegionType, dbRegionType {
	PyObject *obj;
	if ($1.getValue() == odb::dbRegionType::Value::INCLUSIVE) {
		obj = PyString_FromString("INCLUSIVE");
	} else if ($1.getValue() == odb::dbRegionType::Value::EXCLUSIVE) {
		obj = PyString_FromString("EXCLUSIVE");
	} else if ($1.getValue() == odb::dbRegionType::Value::SUGGESTED) {
		obj = PyString_FromString("SUGGESTED");
	}
	$result=obj;
}
%typemap(in) odb::dbRegionType, dbRegionType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "INCLUSIVE") == 0) {
		$1 = odb::dbRegionType::Value::INCLUSIVE;
	} else if (strcasecmp(str, "EXCLUSIVE") == 0) {
		$1 = odb::dbRegionType::Value::EXCLUSIVE;
	} else if (strcasecmp(str, "SUGGESTED") == 0) {
		$1 = odb::dbRegionType::Value::SUGGESTED;
	}
}
%typemap(typecheck) odb::dbRegionType, dbRegionType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "INCLUSIVE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "EXCLUSIVE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "SUGGESTED") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
%typemap(out) odb::dbSourceType, dbSourceType {
	PyObject *obj;
	if ($1.getValue() == odb::dbSourceType::Value::NONE) {
		obj = PyString_FromString("NONE");
	} else if ($1.getValue() == odb::dbSourceType::Value::NETLIST) {
		obj = PyString_FromString("NETLIST");
	} else if ($1.getValue() == odb::dbSourceType::Value::DIST) {
		obj = PyString_FromString("DIST");
	} else if ($1.getValue() == odb::dbSourceType::Value::USER) {
		obj = PyString_FromString("USER");
	} else if ($1.getValue() == odb::dbSourceType::Value::TIMING) {
		obj = PyString_FromString("TIMING");
	} else if ($1.getValue() == odb::dbSourceType::Value::TEST) {
		obj = PyString_FromString("TEST");
	}
	$result=obj;
}
%typemap(in) odb::dbSourceType, dbSourceType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbSourceType::Value::NONE;
	} else if (strcasecmp(str, "NETLIST") == 0) {
		$1 = odb::dbSourceType::Value::NETLIST;
	} else if (strcasecmp(str, "DIST") == 0) {
		$1 = odb::dbSourceType::Value::DIST;
	} else if (strcasecmp(str, "USER") == 0) {
		$1 = odb::dbSourceType::Value::USER;
	} else if (strcasecmp(str, "TIMING") == 0) {
		$1 = odb::dbSourceType::Value::TIMING;
	} else if (strcasecmp(str, "TEST") == 0) {
		$1 = odb::dbSourceType::Value::TEST;
	}
}
%typemap(typecheck) odb::dbSourceType, dbSourceType {
	char *str = PyString_AsString(PyUnicode_AsASCIIString($input));
	bool found = false;
	if (str) {
		if (strcasecmp(str, "NONE") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "NETLIST") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "DIST") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "USER") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "TIMING") == 0) {
			found = true;
		} 	else if (strcasecmp(str, "TEST") == 0) {
			found = true;
		}
	}
	if (found) {
		$1 = 1;
	} else {
		$1 = 0;
	}
}
