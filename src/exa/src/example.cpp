// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "exa/example.h"

#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "observer.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"

namespace exa {

// These need to be defined in the cpp due to the smart pointer in
// example.h to Observer which is only declared there.
Example::Example(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

Example::~Example() = default;

// Checks that a block exists and errors if not.  error() will throw an
// exception with the message.
odb::dbBlock* Example::getBlock()
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    logger_->error(utl::EXA, 2, "No chip exists.");
  }

  odb::dbBlock* block = chip->getBlock();
  if (!block) {
    logger_->error(utl::EXA, 3, "No block exists.");
  }

  return block;
}

// The example operation.
void Example::makeInstance(const char* name)
{
  logger_->info(utl::EXA, 1, "Making an example instance named {}", name);

  odb::dbBlock* block = getBlock();

  // Find an arbitrary master to instantiate
  odb::dbMaster* master = nullptr;
  for (odb::dbLib* lib : db_->getLibs()) {
    if (!lib->getMasters().empty()) {
      master = *lib->getMasters().begin();
      break;
    }
  }

  if (!master) {
    logger_->error(utl::EXA, 4, "No master found.");
  }

  // Make an instance and mark the instance as placed.  The default
  // is at (0, 0) with R0 orientation.
  odb::dbInst* inst = odb::dbInst::create(block, master, name);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // Notify the observer that something interesting has happened.
  if (debug_observer_) {
    debug_observer_->makeInstance(inst);
  }
}

void Example::setDebug(std::unique_ptr<Observer>& observer)
{
  debug_observer_ = std::move(observer);
}

void Example::dbLogTest(int num_threads,
                          int num_entries,
                          int num_chunks)
{
  // Create a thread pool to exercise fork-join patterns.
  // With 0 threads the pool runs everything inline on the caller thread,
  // which makes the exercise deterministic but still path-covers the APIs.
  //
  // The SQLite log database must already be started by Tcl
  // (utl::start_log_db) before calling this command, and stopped
  // afterwards (utl::stop_log_db).
  utl::ThreadPool pool(num_threads);

  // Both tables share the same idx range [0, num_entries) so they can
  // be cross-indexed via a JOIN on the idx column.
  std::vector<int> indices(num_entries);
  std::iota(indices.begin(), indices.end(), 0);

  // -------------------------------------------------------------------
  // 1.  logToDb  —  parallelFor that logs one row per task.
  //     Schema: idx (int), value_a (double).
  //     TableId is 100, table name "first_placeholder_table".
  // -------------------------------------------------------------------
  pool.parallelFor(indices, [&](int i) {
    logger_->logToDb<utl::FixedString{"idx, value_a"}>(
        utl::EXA, 100, "first_placeholder_table", i, i * 3.14);
  });

  // -------------------------------------------------------------------
  // 2.  logToDbBulk  —  parallelized: split into chunks and submit
  //     each chunk as a separate task, exercising concurrent bulk
  //     inserts from multiple threads.
  //     Schema: idx (int), value_b (double).
  //     TableId is 101, table name "second_placeholder_table".
  // -------------------------------------------------------------------
  const int chunk_size = num_entries / num_chunks;
  std::vector<double> values_b(num_entries);
  for (int i = 0; i < num_entries; ++i) {
    values_b[i] = -1.0 + i * 0.01;
  }

  std::vector<utl::ThreadPoolFuture<void>> bulk_futures;
  for (int c = 0; c < num_chunks; ++c) {
    const int start = c * chunk_size;
    bulk_futures.push_back(pool.submit([&, start]() {
      logger_->logToDbBulk<utl::FixedString{"idx, value_b"}>(
          utl::EXA, 101, "second_placeholder_table", chunk_size,
          indices.begin() + start, values_b.begin() + start);
    }));
  }

  // -------------------------------------------------------------------
  // 3.  logToDbMetadata  —  parallelFor writing key-value metadata rows
  //     into the special "metadata" table.
  // -------------------------------------------------------------------
  std::vector<std::pair<std::string, std::string>> meta_entries
      = {{"tool_version", "1.0"},
         {"design_name", "test_design"},
         {"num_entries", std::to_string(num_entries)}};

  pool.parallelFor(meta_entries, [&](const auto& kv) {
    logger_->logToDbMetadata(utl::EXA, kv.first, kv.second);
  });

  // Wait for all bulk tasks to finish (parallelFor already waited inside).
  for (auto& f : bulk_futures) {
    f.get();
  }
}

}  // namespace exa
