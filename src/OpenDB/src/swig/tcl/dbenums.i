
%typemap(out) odb::dbOrientType, dbOrientType {
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbOrientType::Value::R0:
			obj = Tcl_NewStringObj("R0", -1);
			break;
	 	case odb::dbOrientType::Value::R90:
			obj = Tcl_NewStringObj("R90", -1);
			break;
	 	case odb::dbOrientType::Value::R180:
			obj = Tcl_NewStringObj("R180", -1);
			break;
	 	case odb::dbOrientType::Value::R270:
			obj = Tcl_NewStringObj("R270", -1);
			break;
	 	case odb::dbOrientType::Value::MY:
			obj = Tcl_NewStringObj("MY", -1);
			break;
	 	case odb::dbOrientType::Value::MYR90:
			obj = Tcl_NewStringObj("MYR90", -1);
			break;
	 	case odb::dbOrientType::Value::MX:
			obj = Tcl_NewStringObj("MX", -1);
			break;
	 	case odb::dbOrientType::Value::MXR90:
			obj = Tcl_NewStringObj("MXR90", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbOrientType, dbOrientType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
%typemap(typecheck) odb::dbOrientType, dbOrientType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbSigType::Value::SIGNAL:
			obj = Tcl_NewStringObj("SIGNAL", -1);
			break;
	 	case odb::dbSigType::Value::POWER:
			obj = Tcl_NewStringObj("POWER", -1);
			break;
	 	case odb::dbSigType::Value::GROUND:
			obj = Tcl_NewStringObj("GROUND", -1);
			break;
	 	case odb::dbSigType::Value::CLOCK:
			obj = Tcl_NewStringObj("CLOCK", -1);
			break;
	 	case odb::dbSigType::Value::ANALOG:
			obj = Tcl_NewStringObj("ANALOG", -1);
			break;
	 	case odb::dbSigType::Value::RESET:
			obj = Tcl_NewStringObj("RESET", -1);
			break;
	 	case odb::dbSigType::Value::SCAN:
			obj = Tcl_NewStringObj("SCAN", -1);
			break;
	 	case odb::dbSigType::Value::TIEOFF:
			obj = Tcl_NewStringObj("TIEOFF", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbSigType, dbSigType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbIoType::Value::INPUT:
			obj = Tcl_NewStringObj("INPUT", -1);
			break;
	 	case odb::dbIoType::Value::OUTPUT:
			obj = Tcl_NewStringObj("OUTPUT", -1);
			break;
	 	case odb::dbIoType::Value::INOUT:
			obj = Tcl_NewStringObj("INOUT", -1);
			break;
	 	case odb::dbIoType::Value::FEEDTHRU:
			obj = Tcl_NewStringObj("FEEDTHRU", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbIoType, dbIoType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbPlacementStatus::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbPlacementStatus::Value::UNPLACED:
			obj = Tcl_NewStringObj("UNPLACED", -1);
			break;
	 	case odb::dbPlacementStatus::Value::SUGGESTED:
			obj = Tcl_NewStringObj("SUGGESTED", -1);
			break;
	 	case odb::dbPlacementStatus::Value::PLACED:
			obj = Tcl_NewStringObj("PLACED", -1);
			break;
	 	case odb::dbPlacementStatus::Value::LOCKED:
			obj = Tcl_NewStringObj("LOCKED", -1);
			break;
	 	case odb::dbPlacementStatus::Value::FIRM:
			obj = Tcl_NewStringObj("FIRM", -1);
			break;
	 	case odb::dbPlacementStatus::Value::COVER:
			obj = Tcl_NewStringObj("COVER", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbPlacementStatus, dbPlacementStatus {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbMasterType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbMasterType::Value::COVER:
			obj = Tcl_NewStringObj("COVER", -1);
			break;
	 	case odb::dbMasterType::Value::COVER_BUMP:
			obj = Tcl_NewStringObj("COVER_BUMP", -1);
			break;
	 	case odb::dbMasterType::Value::RING:
			obj = Tcl_NewStringObj("RING", -1);
			break;
	 	case odb::dbMasterType::Value::BLOCK:
			obj = Tcl_NewStringObj("BLOCK", -1);
			break;
	 	case odb::dbMasterType::Value::BLOCK_BLACKBOX:
			obj = Tcl_NewStringObj("BLOCK_BLACKBOX", -1);
			break;
	 	case odb::dbMasterType::Value::BLOCK_SOFT:
			obj = Tcl_NewStringObj("BLOCK_SOFT", -1);
			break;
	 	case odb::dbMasterType::Value::PAD:
			obj = Tcl_NewStringObj("PAD", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_INPUT:
			obj = Tcl_NewStringObj("PAD_INPUT", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_OUTPUT:
			obj = Tcl_NewStringObj("PAD_OUTPUT", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_INOUT:
			obj = Tcl_NewStringObj("PAD_INOUT", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_POWER:
			obj = Tcl_NewStringObj("PAD_POWER", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_SPACER:
			obj = Tcl_NewStringObj("PAD_SPACER", -1);
			break;
	 	case odb::dbMasterType::Value::PAD_AREAIO:
			obj = Tcl_NewStringObj("PAD_AREAIO", -1);
			break;
	 	case odb::dbMasterType::Value::CORE:
			obj = Tcl_NewStringObj("CORE", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_FEEDTHRU:
			obj = Tcl_NewStringObj("CORE_FEEDTHRU", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_TIEHIGH:
			obj = Tcl_NewStringObj("CORE_TIEHIGH", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_TIELOW:
			obj = Tcl_NewStringObj("CORE_TIELOW", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_SPACER:
			obj = Tcl_NewStringObj("CORE_SPACER", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_ANTENNACELL:
			obj = Tcl_NewStringObj("CORE_ANTENNACELL", -1);
			break;
	 	case odb::dbMasterType::Value::CORE_WELLTAP:
			obj = Tcl_NewStringObj("CORE_WELLTAP", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP:
			obj = Tcl_NewStringObj("ENDCAP", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_PRE:
			obj = Tcl_NewStringObj("ENDCAP_PRE", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_POST:
			obj = Tcl_NewStringObj("ENDCAP_POST", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_TOPLEFT:
			obj = Tcl_NewStringObj("ENDCAP_TOPLEFT", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_TOPRIGHT:
			obj = Tcl_NewStringObj("ENDCAP_TOPRIGHT", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_BOTTOMLEFT:
			obj = Tcl_NewStringObj("ENDCAP_BOTTOMLEFT", -1);
			break;
	 	case odb::dbMasterType::Value::ENDCAP_BOTTOMRIGHT:
			obj = Tcl_NewStringObj("ENDCAP_BOTTOMRIGHT", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbMasterType, dbMasterType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbTechLayerType::Value::ROUTING:
			obj = Tcl_NewStringObj("ROUTING", -1);
			break;
	 	case odb::dbTechLayerType::Value::CUT:
			obj = Tcl_NewStringObj("CUT", -1);
			break;
	 	case odb::dbTechLayerType::Value::MASTERSLICE:
			obj = Tcl_NewStringObj("MASTERSLICE", -1);
			break;
	 	case odb::dbTechLayerType::Value::OVERLAP:
			obj = Tcl_NewStringObj("OVERLAP", -1);
			break;
	 	case odb::dbTechLayerType::Value::IMPLANT:
			obj = Tcl_NewStringObj("IMPLANT", -1);
			break;
	 	case odb::dbTechLayerType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbTechLayerType, dbTechLayerType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbTechLayerDir::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbTechLayerDir::Value::HORIZONTAL:
			obj = Tcl_NewStringObj("HORIZONTAL", -1);
			break;
	 	case odb::dbTechLayerDir::Value::VERTICAL:
			obj = Tcl_NewStringObj("VERTICAL", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbTechLayerDir, dbTechLayerDir {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbTechLayerDir::Value::NONE;
	} else if (strcasecmp(str, "HORIZONTAL") == 0) {
		$1 = odb::dbTechLayerDir::Value::HORIZONTAL;
	} else if (strcasecmp(str, "VERTICAL") == 0) {
		$1 = odb::dbTechLayerDir::Value::VERTICAL;
	}
}
%typemap(typecheck) odb::dbTechLayerDir, dbTechLayerDir {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbRowDir::Value::HORIZONTAL:
			obj = Tcl_NewStringObj("HORIZONTAL", -1);
			break;
	 	case odb::dbRowDir::Value::VERTICAL:
			obj = Tcl_NewStringObj("VERTICAL", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbRowDir, dbRowDir {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "HORIZONTAL") == 0) {
		$1 = odb::dbRowDir::Value::HORIZONTAL;
	} else if (strcasecmp(str, "VERTICAL") == 0) {
		$1 = odb::dbRowDir::Value::VERTICAL;
	}
}
%typemap(typecheck) odb::dbRowDir, dbRowDir {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbBoxOwner::Value::UNKNOWN:
			obj = Tcl_NewStringObj("UNKNOWN", -1);
			break;
	 	case odb::dbBoxOwner::Value::BLOCK:
			obj = Tcl_NewStringObj("BLOCK", -1);
			break;
	 	case odb::dbBoxOwner::Value::INST:
			obj = Tcl_NewStringObj("INST", -1);
			break;
	 	case odb::dbBoxOwner::Value::BTERM:
			obj = Tcl_NewStringObj("BTERM", -1);
			break;
	 	case odb::dbBoxOwner::Value::VIA:
			obj = Tcl_NewStringObj("VIA", -1);
			break;
	 	case odb::dbBoxOwner::Value::OBSTRUCTION:
			obj = Tcl_NewStringObj("OBSTRUCTION", -1);
			break;
	 	case odb::dbBoxOwner::Value::SWIRE:
			obj = Tcl_NewStringObj("SWIRE", -1);
			break;
	 	case odb::dbBoxOwner::Value::BLOCKAGE:
			obj = Tcl_NewStringObj("BLOCKAGE", -1);
			break;
	 	case odb::dbBoxOwner::Value::MASTER:
			obj = Tcl_NewStringObj("MASTER", -1);
			break;
	 	case odb::dbBoxOwner::Value::MPIN:
			obj = Tcl_NewStringObj("MPIN", -1);
			break;
	 	case odb::dbBoxOwner::Value::TECH_VIA:
			obj = Tcl_NewStringObj("TECH_VIA", -1);
			break;
	 	case odb::dbBoxOwner::Value::REGION:
			obj = Tcl_NewStringObj("REGION", -1);
			break;
	 	case odb::dbBoxOwner::Value::BPIN:
			obj = Tcl_NewStringObj("BPIN", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbBoxOwner, dbBoxOwner {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbPolygonOwner::Value::UNKNOWN:
			obj = Tcl_NewStringObj("UNKNOWN", -1);
			break;
	 	case odb::dbPolygonOwner::Value::BPIN:
			obj = Tcl_NewStringObj("BPIN", -1);
			break;
	 	case odb::dbPolygonOwner::Value::OBSTRUCTION:
			obj = Tcl_NewStringObj("OBSTRUCTION", -1);
			break;
	 	case odb::dbPolygonOwner::Value::SWIRE:
			obj = Tcl_NewStringObj("SWIRE", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbPolygonOwner, dbPolygonOwner {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbWireType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbWireType::Value::COVER:
			obj = Tcl_NewStringObj("COVER", -1);
			break;
	 	case odb::dbWireType::Value::FIXED:
			obj = Tcl_NewStringObj("FIXED", -1);
			break;
	 	case odb::dbWireType::Value::ROUTED:
			obj = Tcl_NewStringObj("ROUTED", -1);
			break;
	 	case odb::dbWireType::Value::SHIELD:
			obj = Tcl_NewStringObj("SHIELD", -1);
			break;
	 	case odb::dbWireType::Value::NOSHIELD:
			obj = Tcl_NewStringObj("NOSHIELD", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbWireType, dbWireType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
%typemap(typecheck) odb::dbWireType, dbWireType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbWireShapeType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbWireShapeType::Value::RING:
			obj = Tcl_NewStringObj("RING", -1);
			break;
	 	case odb::dbWireShapeType::Value::PADRING:
			obj = Tcl_NewStringObj("PADRING", -1);
			break;
	 	case odb::dbWireShapeType::Value::BLOCKRING:
			obj = Tcl_NewStringObj("BLOCKRING", -1);
			break;
	 	case odb::dbWireShapeType::Value::STRIPE:
			obj = Tcl_NewStringObj("STRIPE", -1);
			break;
	 	case odb::dbWireShapeType::Value::FOLLOWPIN:
			obj = Tcl_NewStringObj("FOLLOWPIN", -1);
			break;
	 	case odb::dbWireShapeType::Value::IOWIRE:
			obj = Tcl_NewStringObj("IOWIRE", -1);
			break;
	 	case odb::dbWireShapeType::Value::COREWIRE:
			obj = Tcl_NewStringObj("COREWIRE", -1);
			break;
	 	case odb::dbWireShapeType::Value::BLOCKWIRE:
			obj = Tcl_NewStringObj("BLOCKWIRE", -1);
			break;
	 	case odb::dbWireShapeType::Value::BLOCKAGEWIRE:
			obj = Tcl_NewStringObj("BLOCKAGEWIRE", -1);
			break;
	 	case odb::dbWireShapeType::Value::FILLWIRE:
			obj = Tcl_NewStringObj("FILLWIRE", -1);
			break;
	 	case odb::dbWireShapeType::Value::DRCFILL:
			obj = Tcl_NewStringObj("DRCFILL", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbWireShapeType, dbWireShapeType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbSiteClass::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbSiteClass::Value::PAD:
			obj = Tcl_NewStringObj("PAD", -1);
			break;
	 	case odb::dbSiteClass::Value::CORE:
			obj = Tcl_NewStringObj("CORE", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbSiteClass, dbSiteClass {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "NONE") == 0) {
		$1 = odb::dbSiteClass::Value::NONE;
	} else if (strcasecmp(str, "PAD") == 0) {
		$1 = odb::dbSiteClass::Value::PAD;
	} else if (strcasecmp(str, "CORE") == 0) {
		$1 = odb::dbSiteClass::Value::CORE;
	}
}
%typemap(typecheck) odb::dbSiteClass, dbSiteClass {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbOnOffType::Value::OFF:
			obj = Tcl_NewStringObj("OFF", -1);
			break;
	 	case odb::dbOnOffType::Value::ON:
			obj = Tcl_NewStringObj("ON", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbOnOffType, dbOnOffType {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "OFF") == 0) {
		$1 = odb::dbOnOffType::Value::OFF;
	} else if (strcasecmp(str, "ON") == 0) {
		$1 = odb::dbOnOffType::Value::ON;
	}
}
%typemap(typecheck) odb::dbOnOffType, dbOnOffType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbClMeasureType::Value::EUCLIDEAN:
			obj = Tcl_NewStringObj("EUCLIDEAN", -1);
			break;
	 	case odb::dbClMeasureType::Value::MAXXY:
			obj = Tcl_NewStringObj("MAXXY", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbClMeasureType, dbClMeasureType {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "EUCLIDEAN") == 0) {
		$1 = odb::dbClMeasureType::Value::EUCLIDEAN;
	} else if (strcasecmp(str, "MAXXY") == 0) {
		$1 = odb::dbClMeasureType::Value::MAXXY;
	}
}
%typemap(typecheck) odb::dbClMeasureType, dbClMeasureType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbJournalEntryType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbJournalEntryType::Value::OWNER:
			obj = Tcl_NewStringObj("OWNER", -1);
			break;
	 	case odb::dbJournalEntryType::Value::ADD:
			obj = Tcl_NewStringObj("ADD", -1);
			break;
	 	case odb::dbJournalEntryType::Value::DESTROY:
			obj = Tcl_NewStringObj("DESTROY", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbJournalEntryType, dbJournalEntryType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbDirection::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbDirection::Value::NORTH:
			obj = Tcl_NewStringObj("NORTH", -1);
			break;
	 	case odb::dbDirection::Value::EAST:
			obj = Tcl_NewStringObj("EAST", -1);
			break;
	 	case odb::dbDirection::Value::SOUTH:
			obj = Tcl_NewStringObj("SOUTH", -1);
			break;
	 	case odb::dbDirection::Value::WEST:
			obj = Tcl_NewStringObj("WEST", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbDirection, dbDirection {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbRegionType::Value::INCLUSIVE:
			obj = Tcl_NewStringObj("INCLUSIVE", -1);
			break;
	 	case odb::dbRegionType::Value::EXCLUSIVE:
			obj = Tcl_NewStringObj("EXCLUSIVE", -1);
			break;
	 	case odb::dbRegionType::Value::SUGGESTED:
			obj = Tcl_NewStringObj("SUGGESTED", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbRegionType, dbRegionType {
	char *str = Tcl_GetStringFromObj($input, 0);
	if (strcasecmp(str, "INCLUSIVE") == 0) {
		$1 = odb::dbRegionType::Value::INCLUSIVE;
	} else if (strcasecmp(str, "EXCLUSIVE") == 0) {
		$1 = odb::dbRegionType::Value::EXCLUSIVE;
	} else if (strcasecmp(str, "SUGGESTED") == 0) {
		$1 = odb::dbRegionType::Value::SUGGESTED;
	}
}
%typemap(typecheck) odb::dbRegionType, dbRegionType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	Tcl_Obj *obj = nullptr;
	switch ($1.getValue()) {
		case odb::dbSourceType::Value::NONE:
			obj = Tcl_NewStringObj("NONE", -1);
			break;
	 	case odb::dbSourceType::Value::NETLIST:
			obj = Tcl_NewStringObj("NETLIST", -1);
			break;
	 	case odb::dbSourceType::Value::DIST:
			obj = Tcl_NewStringObj("DIST", -1);
			break;
	 	case odb::dbSourceType::Value::USER:
			obj = Tcl_NewStringObj("USER", -1);
			break;
	 	case odb::dbSourceType::Value::TIMING:
			obj = Tcl_NewStringObj("TIMING", -1);
			break;
	 	case odb::dbSourceType::Value::TEST:
			obj = Tcl_NewStringObj("TEST", -1);
			break;
	}
	Tcl_SetObjResult(interp, obj);
}
%typemap(in) odb::dbSourceType, dbSourceType {
	char *str = Tcl_GetStringFromObj($input, 0);
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
	char *str = Tcl_GetStringFromObj($input, 0);
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
