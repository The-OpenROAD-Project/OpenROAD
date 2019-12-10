set db [dbDatabase_create]

set libs [odb_read_lef $db $env(LEF_FILES)]
set chip [odb_read_def $libs $env(DEF_FILE)]

odb_export_db $db $env(DESIGN).fp.odb

