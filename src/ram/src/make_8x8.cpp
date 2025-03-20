#include "ord/OpenRoad.hh"   // Core OpenROAD interface
#include "ram/ram.h"         // RAM generator header
#include "utl/Logger.h"      // Logging
#include "odb/db.h"          // Database access

int main(int argc, char* argv[]) {
    // Initialize OpenROAD
    ord::OpenRoad* openroad = ord::OpenRoad::openRoad();  // OpenRoad singleton
    openroad->init(nullptr, nullptr, nullptr);           // Initialize tools

    // Access the RAM generation tool
    ram::RamGen* ram_gen = openroad->getRamGen();        // Get RAM generator
    utl::Logger* logger = openroad->getLogger();         // Access logger
    odb::dbDatabase* db = openroad->getDb();             // Access design database

    // Example inputs for the RAM generation function
    const int bytes_per_word = 1;
    const int word_count = 8;
    const int read_ports = 2;

    const char* storage_cell_name = "sky130_fd_sc_hd__dlxtp_1";
    odb::dbMaster* storage_cell = db->findMaster(storage_cell_name);

    // Error check
    if (!storage_cell) {
        logger->error(utl::RAM, 199, "Storage cell {} not found", storage_cell_name);
        return 1;
    }

    // Call the RAM generator
    ram_gen->generate(bytes_per_word, word_count, read_ports, storage_cell, nullptr, nullptr);

    logger->info(utl::RAM, 288, "RAM generation completed successfully.");

    return 0;
}
