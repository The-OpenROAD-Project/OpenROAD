%{
#include "defin.h"

bool
db_diff(odb::dbDatabase *db1,
	odb::dbDatabase *db2)
{
  // Sadly the diff report is too implementation specific to reveal much about
  // the structural differences.
  bool diffs = odb::dbDatabase::diff(db1, db2, nullptr, 2);
  if (diffs) {
    printf("Differences found.\n");
    odb::dbChip *chip1 = db1->getChip();
    odb::dbChip *chip2 = db2->getChip();
    odb::dbBlock *block1 = chip1->getBlock();
    odb::dbBlock *block2 = chip2->getBlock();

    int inst_count1 = block1->getInsts().size();
    int inst_count2 = block2->getInsts().size();
    if (inst_count1 != inst_count2)
      printf(" instances %d != %d.\n", inst_count1, inst_count2);

    int pin_count1 = block1->getBTerms().size();
    int pin_count2 = block2->getBTerms().size();
    if (pin_count1 != pin_count2)
      printf(" pins %d != %d.\n", pin_count1, pin_count2);

    int net_count1 = block1->getNets().size();
    int net_count2 = block2->getNets().size();
    if (net_count1 != net_count2)
      printf(" nets %d != %d.\n", net_count1, net_count2);
  }
  else
    printf("No differences found.\n");
  return diffs;
}

bool
db_def_diff(odb::dbDatabase *db1,
	    const char *def_filename)
{
  // Copy the database to get the tech and libraries.
  odb::dbDatabase *db2 = odb::dbDatabase::duplicate(db1);
  odb::dbChip *chip2 = db2->getChip();
  if (chip2)
    odb::dbChip::destroy(chip2);

  odb::defin def_reader(db2);
  std::vector<odb::dbLib *> search_libs;
  for (odb::dbLib *lib : db2->getLibs())
    search_libs.push_back(lib);
  def_reader.createChip(search_libs, def_filename);
  if (db2->getChip())
    return db_diff(db1, db2);
  else
    return false;
}

%}

bool
db_diff(odb::dbDatabase *db1,
	odb::dbDatabase *db2);
bool
db_def_diff(odb::dbDatabase *db1,
	    const char *def_filename);
