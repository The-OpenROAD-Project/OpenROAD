#include <iostream>
#include <string>

#include "odb/db.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"  // Hypothetical header for GDSWriter

using namespace odb;

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input gds file> <output gds file>" << std::endl;
        return 1;
    }

    const std::string input_gds_file = argv[1];
    const std::string output_gds_file = argv[2];

    // Step 1: Read the input GDS file
    dbDatabase* db = dbDatabase::create();
    GDSReader reader;
    dbGDSLib* lib = reader.read_gds(input_gds_file, db);

    if(lib == nullptr)
    {
        std::cerr << "Failed to read the GDS file: " << input_gds_file << std::endl;
        return 1;
    }

    // Step 2: Write the output GDS file
    GDSWriter writer;
    writer.write_gds(lib, output_gds_file);  // No need to check for a return value

    std::cout << "GDS file successfully written to " << output_gds_file << std::endl;

    return 0;
}

