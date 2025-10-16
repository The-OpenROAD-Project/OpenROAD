# Error message number:
#   6000 - out of memory
#   6010 - defiBlockage.cpp
#   6020 - defiComponent.cpp
#   6030 - defiFPC.cpp
#   6040 - defiFill.cpp
#   6050 - defiGroup.cpp
#   6060 - defiIOTiming.cpp
#   6070 - defiMisc.cpp
#   6080 - defiNet.cpp
#   6090 - defiNonDefault.cpp
#   6100 - defiPartition.cpp
#   6110 - defiPinCap.cpp
#   6120 - defiPinProp.cpp
#   6130 - defiRegion.cpp
#   6140 - defiRowTrack.cpp
#   6150 - defiScanchain.cpp
#   6160 - defiSlot.cpp
#   6170 - defiTimingDisable.cpp
#   6180 - defiVia.cpp
#   6200 - defiAssertion.cpp
# 5000 - def reader, defrReader.cpp
# 5500 - lex.cpph, yyerror
# 6000 - def parser, error, lex.cpph, def.y (CALLBACK and CHKERR)
# 6500 - def parser, error, def.y
# 7000 - def parser, warning, lex.cpph
# 7500 - def parser, warning, lef.y
# 8000 - def parser, info, lex.cpph
# 9000 - def writer
# emsMkError DEF -b defMsgTable -m def.msg -e -n
5000 "The 'defrRead' function has been called before the 'defrInit' function.\nThe 'defrInit' function should be called prior to the 'defrRead' function."
5001 "DEF statement found in the def file with no callback set."
6000 "The syntax for an ALIAS statement is \"&ALIAS aliasName = aliasDefinition &ENDALIAS\". '=' is missing after the aliasName."
6001 "End of file is reached while parsing in the middle of an ALIAS statement.\nReview you def file and add '&ENDALIAS' in the ALIAS statement."
6002 "The def file is incomplete."
6003 "The BEGINEXT tag is missing in the DEF file. Include the tag and then try again."
6004 "The BEGINEXT tag is empty. Specify a value for the tag and try again."
6005 "The '\"' is missing within the tag. Specify the '\"' in the tag and then try again."
6006 "The ending '\"' is missing in the tag. Specify the ending '\"' in the tag and then try again."
6007 "The ENDEXT statement is missing in the DEF file. Include the statement and then try again."
6008 "Invalid characters found in \'%s\'.\nThese characters might be using the character types other than English.\nCreate characters by specifying valid characters types."
6010 "An error has been reported in callback."
6011 "Too many syntax errors have been reported."
6030 "Invalid direction specified with FPC name. The valid direction is either 'H' or 'V'. Specify a valid vale and then try again."
6060 "Invalid value specified for IOTIMING rise/fall. The valid value for rise is 'R' and for fall is 'F'. Specify a valid value and then try again."
6080 "An internal error has occurred. The index number for the SUBNET wires array is less then or equal to 0.\nContact Cadence Customer Support with this error information."
6081 "An internal error has occurred. The index number for the NET PATH wires array is less then or equal to 0.\nContact Cadence Customer Support with this error information."
6082 "An internal error has occurred. The index number for the NET SHIELDPATH wires array is less then or equal to 0.\nContact Cadence Customer Support with this error information."
6083 "The index number %d specified for the NET INSTANCE is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6084 "The index number %d specified for the NET PIN is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6085 "The index number %d specified for the NET POLYGON is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6086 "The index number %d specified for the NET RECTANGLE is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6090 "The index number %d specified for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6091 "The index number %d specified for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again."
6100 "The value spefified for PARTITION SETUP is invalid. The valid value for SETUP is 'R' or 'F'. Specify a valid value for SETUP and then try again."
6101 "The value spefified for PARTITION HOLD is invalid. The valid value for HOLD is 'R' or 'F'. Specify a valid value for HOLD and then try again."
6120 "The index number %d specified for the PIN PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again."
6130 "The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again."
6131 "The index number %d specified for the REGION RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again."
6140 "The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again."
6150 "The START statement in the SCANCHAINS has defined more than one time in the SCANCHAINS statement.\nUpdate the DEF file to only one START statement and then try again."
6151 "The STOP statment in the SCANCHAINS has defined more than one time in the SCANCHAINS statement.\nUpdate the DEF file to only one STOP statement and then try again."
6160 "The index number %d specified for the SLOT RECTANGLE is invalid.\nValid index number is from 0 to %d. Specify a valid index number and then try again."
6170 "The TimingDisable type is invalid. The valid types are FROMPIN, & THRUPIN. Specify the valid type and then try again."
6180 "The index number %d specified for the VIA POLYGON is invalid.\nValid index is from 0 to %d. Specify a valid index number and then try again"
6200 "The ASSERTION statement is invalid because it has an invalid operand rule.\nValid operand rule is either NET or PATH. Specify a valid operand and then try again."
6201 "Unable to process the DEF file. Both WIREDLOGIC and DELAY statements are defined in constraint/assertion.\nUpdate the DEF file to define either a WIREDLOGIC or DELAY statement only."
6202 "Unable to process the DEF file. Both SUM and DIFF statements are defined in constraint/assertion.\nUpdate the DEF file to define either a SUM or DIFF statement only."
6501 "An error has been found while processing the DEF file '%s'\nUnit %d is a 5.6 or later syntax. Define the DEF file as 5.6 and then try again."
6502 "The value %d defined for DEF UNITS DISTANCE MICRON is invalid\n. The valid values are 100, 200, 1000, 2000, 10000, or 20000. Specify a valid value and then try again."
6503 "The execution has been stopped because the DEF parser 5.7 does not support DEF file with version %s.\nUpdate your DEF file to version 5.7 or earlier."
6504 "Def parser version 5.7 and later does not support NAMESCASESENSITIVE OFF.\nEither remove this optional construct or set it to ON."
6505 "The NONDEFAULTRULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6506 "The NETEXPR statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6507 "The SUPPLYSENSITIVITY statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6508 "The GROUNDSENSITIVITY statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6509 "The POLYGON statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6510 "The ANTENNAPINPARTIALMETALAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g."
6511 "The ANTENNAPINPARTIALMETALSIDEAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6512 "The ANTENNAPINGATEAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6513 "The ANTENNAPINDIFFAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6514 "The ANTENNAPINMAXAREACAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6515 "The ANTENNAPINMAXSIDEAREACAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6516 "The ANTENNAPINPARTIALCUTAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6517 "The ANTENNAPINMAXCUTCAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g"
6518 "The ANTENNAMODEL statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g"
6519 "The SPACING statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6520 "The DESIGNRULEWIDTH statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6523 "Invalid ROW statement defined in the DEF file. The DO statement which is required in the ROW statement is not defined.\nUpdate your DEF file with a DO statement."
6524 "Invalid syntax specified. The valid syntax is either \"DO 1 BY num or DO num BY 1\". Specify the valid syntax and try again."
6525 "The DO number %g in TRACK is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again."
6526 "The STEP number %g in TRACK is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again."
6527 "The DO number %g in GCELLGRID is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again."
6528 "The STEP number %g in GCELLGRID is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again."
6529 "The HALO statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6530 "The FIXEDBUMP statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g"
6531 "The layerName which is required in path is missing. Include the layerName in the path and then try again."
6532 "The VIA DO statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g"
6533 "Either the numX or numY in the VIA DO statement has invalid value. The value specified is 0.\nUpdate your DEF file with the correct value and then try again."
6534 "The STYLE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6535 "The POLYGON statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6536 "The RECT statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6537 "The value %s for statement VOLTAGE is invalid. The value can only be integer.\nSpecify a valid value in units of millivolts"
6538 "The PARTITION statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g"
6539 "Invalid BLOCKAGE statement defined in the DEF file. The BLOCKAGE statment has both the LAYER and the PLACEMENT statements defined.\nUpdate your DEF file to have either BLOCKAGE or PLACEMENT statement only."
6540 "The SPACING statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6541 "The SPACING statement is defined in the LAYER statement,\nbut there is already either a SPACING statement or DESIGNRULEWIDTH  statement has defined in the LAYER statement.\nUpdate your DEF file to have either SPACING statement or a DESIGNRULEWIDTH statement."
6542 "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer."
6543 "The defined BLOCKAGES PLACEMENT statement has either COMPONENT, PUSHDOWN, SOFT or PARTIAL defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES PLACEMENT statement."
6544 "A POLYGON statement is defined in the BLOCKAGE statement,\nbut it is not defined in the BLOCKAGE LAYER statement.\nUpdate your DEF file to either remove the POLYGON statement from the BLOCKAGE statement or\ndefine the POLYGON statement in a BLOCKAGE LAYER statement."
6545 "The NONDEFAULTRULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g."
6546 "The STYLES statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g"
6547 "The PLACEMENT SOFT is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6548 "The PARTIAL is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6549 "The EXCEPTPGNET is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6550 "The HALO SOFT is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6551 "The ROUTEHALO is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6552 "The FILLWIREOPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6553 "The LAYER OPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6554 "The VIA OPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6555 "The PORT in PINS is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g."
6556 "The PIN VIA statement is available in version 5.7 and later.\nHowever, your DEF file is defined with version %g."
6557 "The VIARULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g",
6558 "The FREQUENCY statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g"
6559 "The ROWCOL statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement."
6560 "The ORIGIN statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement."
6561 " The OFFSET statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement."
6562 "The PATTERN statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement."
6563 "The TYPE statement already exists. It has been defined in the REGION statement."
6564 "POLYGON statement in FILLS LAYER is a version 5.6 and later syntax.\nYour def file is defined with version %g",
7000 "The specified string has exceeded 4096 characters. The extra characters will be truncated. Specify a string less than or equal to 4096 characters."
7010 "The PropName %s is not defined for %s."
7011 "The NAMESCASESENSITIVE statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement."
7012 "No VERSION statement found, using the default value %2g."
7013 "The DEF file is invalid if NAMESCASESENSITIVE is undefined.\nNAMESCASESENSITIVE is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the NAMESCASESENSITIVE statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual."
7014 "The DEF file is invalid if BUSBITCHARS is undefined.\nBUSBITCHARS is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the BUSBITCHARS statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual."
7015 "The DEF file is invalid if DIVIDERCHAR is undefined.\nDIVIDERCHAR is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the DIVIDERCHAR statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual."
7016 "DESIGN is a mandatory statement in the DEF file. Ensure that it exists in the file."
7017 "The DEFAULTCAP statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement."
7018 "The DO statement in the ROW statement with the name %s has invalid syntax.\nThe valid syntax is \"DO numX BY 1 STEP spaceX 0 | DO 1 BY numY STEP 0 spaceY\".\nSpecify the valid syntax and try again."
7019 "The PATTERNNAME statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement."
7020 "The REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7021 "The FOREIGN statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement."
7022 "In the COMPONENT UNPLACED statement, the point and orient are invalid in version 5.4 and later.\nThe DEF parser will ignore this statement."
7023 "The SPECIAL NET statement, with type %s, does not have any net statement defined.\nThe DEF parser will ignore this statement."
7024 "The ESTCAP statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7025 "The SPECIAL NET SHIELD statement, does not have any shield net statement defined.\nThe DEF parser will ignore this statement."
7026 "The WIDTH statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7027 "The GROUP REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7028 "The GROUP SOFT MAXX statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7029 "The GROUP SOFT MAXY statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7030 "The GROUP SOFT MAXHALFPERIMETER statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement."
7031 "The ASSERTIONS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement."
7032 "The CONSTRAINTS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement."
7035 "The IOTIMINGS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement."
7500 "Unable to open the file defRWarning.log in %s.\nWarning messages will not be written out in the log file.\nCheck if you have write permission on the directory."
8000 "The data still exists after the END DESIGN statement. The DEF parser will ignore this data."
8500 "Unable to open the file defRWarning.log in %s.\nInfo messages will not be written out in the log file.\nCheck if you have write permission on the directory."
9000 "The DEF writer has detected that the function defwInitCbk has already been called and you are trying to call defwInit.\nOnly defwInitCbk or defwInit can be called but not both.\nUpdate your program and then try again."
9001 "The DEF writer has detected that the function defwInit has already been called and you are trying to call defwInitCbk.\nOnly defwInitCbk or defwInit can be called but not both.\nUpdate your program and then try again."
9010 "The function defwWrite is called before the function defwInitCbk.\nYou need to call defwInitCbk before calling any other functions.\nUpdate your program and then try again."
9011 "You program has called the function defwInit to initialize the writer.\nIf you want to use the callback option you need to use the function defwInitCbk."
9012 "You are calling the function defwPrintUnusedCallbacks but you did call the function defwSetRegisterUnusedCallbacks which is required before you can call defwPrintUnusedCallbacks."
