# Adding new fields in DB Object

For example `add_pitchDiag` in object `DbTechLayer`.

|   | Action                                                 | File              | Source Code                                                                        |
|---|--------------------------------------------------------|-------------------|------------------------------------------------------------------------------------|
| 1 | Add Fields at the .h file                              | `dbTechLayer.h`   |                                                                                    |
| 2 | Define a keyword for db rev number                     | `dbDatabase.h`    | `#define ADS_DB_DF58 52`                                                           |
| 3 | Set the current rev number same as                     | `dbDatabase.h`    | `#define ADS_DB_SCHEMA_MINOR 52`                                                   |
| 4 | Stream in new fields Conditionally upon Schema number  | `dbTechLayer.cpp` | `if ( stream.getDatabase()->isSchema(ADS_DB_DF58) ) { stream >> layer._pitchDiag;` |
| 5 | Stream out new fields Conditionally upon Schema number | `dbTechLayer.cpp` | `if ( stream.getDatabase()->isSchema(ADS_DB_DF58) ) { stream << layer._pitchDiag;` |
| 6 | Conditionally Diff new fields                          | `dbTechLayer.cpp` | `if ( stream.getDatabase()->isSchema(ADS_DB_DF58) ) { DIFF_FIELD(_pitchDiag);`     |
| 7 | Conditionally Diff Out new fields                      | `dbTechLayer.cpp` | `if ( stream.getDatabase()->isSchema(ADS_DB_DF58) ) { DIFF_OUT_FIELD(_pitchDiag);` |
| 8 | Created access APIs to the fields                      | `dbTechLayer.cpp` | `"dbTechLayer::getPitchDiag(), dbTechLayer::setPitchDiag( int pitch )"`            |
| 9 | Add new APIs in include/db.h                           | db.h              | `class dbTechLayer`                                                                |
