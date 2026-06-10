// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "utl/Logger.h"

#include <sqlite3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "CommandLineProgress.h"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "utl/Metrics.h"
#include "utl/Progress.h"
#include "utl/prometheus/metrics_server.h"
#include "utl/prometheus/registry.h"

namespace utl {

Logger::Logger(const char* log_filename, const char* metrics_filename)
{
  progress_ = std::make_unique<CommandLineProgress>(this);

  sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  if (log_filename) {
    sinks_.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename));
  }

  logger_ = std::make_shared<spdlog::logger>(
      "logger", sinks_.begin(), sinks_.end());
  setFormatter();
  logger_->set_level(spdlog::level::level_enum::debug);

  if (metrics_filename) {
    addMetricsSink(metrics_filename);
  }

  metrics_policies_ = MetricsPolicy::makeDefaultPolicies();

  for (auto& counters : message_counters_) {
    for (auto& counter : counters) {
      counter = 0;
    }
  }

  for (auto& levels : message_levels_) {
    for (auto& level : levels) {
      level.store(spdlog::level::off, std::memory_order_relaxed);
    }
  }

  prometheus_registry_ = std::make_shared<PrometheusRegistry>();
}

Logger::~Logger()
{
  stopLogDb();
  finalizeMetrics();
}

void Logger::addMetricsSink(const char* metrics_filename)
{
  metrics_sinks_.emplace_back(metrics_filename);
}

void Logger::removeMetricsSink(const char* metrics_filename)
{
  auto metrics_file = std::ranges::find(metrics_sinks_, metrics_filename);
  if (metrics_file == metrics_sinks_.end()) {
    this->error(UTL, 11, "{} is not a metrics file", metrics_filename);
  }
  flushMetrics();

  metrics_sinks_.erase(metrics_file);
}

// Create initial stuff and launch thread, and await startup.
void Logger::startLogDb(const char* filename)
{
  if (db_ready_) {
    return;
  }

  // Reset startup state
  db_filename_ = filename;
  db_start_error_.clear();
  {
    std::lock_guard<std::mutex> lock(startup_mutex_);
    startup_done_ = false;
  }

  log_db_running_ = true;
  log_db_thread_ = std::thread(&Logger::logDbLoop, this);

  // Wait for the backend thread to open the database and create system
  // tables.
  {
    std::unique_lock<std::mutex> lock(startup_mutex_);
    startup_cv_.wait(lock, [this]() { return startup_done_; });
  }

  if (!db_ready_) {
    // Backend failed to open the database.
    std::string err_msg
        = db_start_error_.empty() ? "unknown error" : db_start_error_;
    log_db_running_ = false;
    if (log_db_thread_.joinable()) {
      log_db_thread_.join();
    }
    this->error(
        UTL, 109, "Failed to open SQLite database {}: {}", filename, err_msg);
  }
  this->info(UTL, 117, "Logging to database: {}", filename);
}

// Flag running as false, and block on thread join.
// FIXME: Fix the corner case where this can be called twice,
// with the second time returning imediately. Not a real problem for most situations.
void Logger::stopLogDb()
{
  if (!log_db_running_) {
    return;
  }
  log_db_running_ = false;
  if (log_db_thread_.joinable()) {
    log_db_thread_.join();
  }
  this->info(UTL, 118, "Stopping database logging.");
  // All SQLite operations (drain, close) are handled inside the backend
  // thread (logDbLoop).  Nothing more to do here.
}

// Free helper utilities. Put in anonymous namespace because I suspect they can be easily replaced
// with stdlib methods.
namespace {

const char* sqlite_type_name(utl::SQLiteType t)
{
  switch (t) {
    case utl::SQLiteType::INTEGER:
      return "INTEGER";
    case utl::SQLiteType::REAL:
      return "REAL";
  }
  return "INTEGER";
}

// Split a comma-separated header string into individual column names,
// trimming leading/trailing whitespace from each.
// FIXME: There might be a way to do this with stdlib things,
// it seems too trivial to not have a nicer way.
std::vector<std::string> split_header(std::string_view header)
{
  std::vector<std::string> fields;
  const char* cur = header.data();
  const char* end = cur + header.size();
  while (cur < end) {
    // Skip leading spaces
    while (cur < end && *cur == ' ') {
      ++cur;
    }
    const char* start = cur;
    // Find next comma or end
    while (cur < end && *cur != ',') {
      ++cur;
    }
    // Trim trailing spaces
    const char* stop = cur;
    while (stop > start && *(stop - 1) == ' ') {
      --stop;
    }
    if (stop > start || (!fields.empty() && stop == start)) {
      fields.emplace_back(start, stop - start);
    }
    if (cur < end) {
      ++cur;  // skip comma
    }
  }
  return fields;
}

std::vector<ColumnDefinition> build_columns_from_runtime(
    std::string_view header,
    const std::vector<SQLiteType>& types)
{
  auto names = split_header(header);
  // HACK: if counts don't match, pad with "unknown"
  std::vector<ColumnDefinition> cols;
  cols.reserve(types.size());
  for (size_t i = 0; i < types.size(); ++i) {
    std::string name = (i < names.size()) ? names[i] : "unknown";
    cols.push_back({std::move(name), types[i]});
  }
  return cols;
}

}  // end anonymous namespace

// logToDb helpers

// Fish out queue from map or return optional.
// FIXME: Might be cleaner to just inline this wherever necessary.
std::optional<std::shared_ptr<AbstractQueue>> Logger::logToDbFindQueue(
    SchemaKey key)
{
  auto map = schema_registry_.get_map();
  auto it = map->find(key);
  if (it != map->end()) {
    return it->second;
  }
  return std::nullopt;
}


// Helper for building a schema and creating a table from source material.
// TODO: Would require changing a bunch of signatures but it would be nice to allow
// The caller to pass some kind of long "table description" string.
SchemaInfo Logger::logToDbBuildSchemaInfo(sqlite3* db,
                                          SchemaKey key,
                                          const std::string& table_name,
                                          std::string_view header,
                                          const std::vector<SQLiteType>& types)
{
  SchemaInfo info;
  info.table_name = table_name;
  info.columns = build_columns_from_runtime(header, types);

  this->info(UTL,
             121,
             "Creating database table {} for {}-{}.",
             info.table_name,
             tool_names_[key.tool],
             key.id);

  // FIXME: as before not sure if there is a better way than string appending
  // Build and execute CREATE TABLE IF NOT EXISTS in one shot.
  std::string create_sql
      = "CREATE TABLE IF NOT EXISTS " + info.table_name + " (";
  for (size_t i = 0; i < info.columns.size(); ++i) {
    if (i > 0) {
      create_sql += ", ";
    }
    create_sql
        += info.columns[i].name + " " + sqlite_type_name(info.columns[i].type);
  }
  create_sql += ");";

  char* err_msg = nullptr;
  int rc = sqlite3_exec(db, create_sql.c_str(), nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    std::string err_str = err_msg ? err_msg : "unknown";
    sqlite3_free(err_msg);
    this->error(UTL,
                110,
                "SQLite error creating table '{}': {}",
                info.table_name,
                err_str);
  }

  // Insert into table_list to track this registered schema.
  std::string col_types, col_names;
  for (size_t i = 0; i < info.columns.size(); ++i) {
    if (i > 0) {
      col_types += ",";
      col_names += ",";
    }
    col_types += sqlite_type_name(info.columns[i].type);
    col_names += info.columns[i].name;
  }
  char* tbl_sql = sqlite3_mprintf(
      "INSERT OR REPLACE INTO table_list VALUES (%d, %d, %Q, %Q, %Q)",
      static_cast<int>(key.tool),
      key.id,
      info.table_name.c_str(),
      col_types.c_str(),
      col_names.c_str());
  if (tbl_sql) {
    char* tbl_err = nullptr;
    if (sqlite3_exec(db, tbl_sql, nullptr, nullptr, &tbl_err) != SQLITE_OK) {
      this->warn(UTL, 115, "Failed to insert into table_list: {}", tbl_err);
      sqlite3_free(tbl_err);
    }
    sqlite3_free(tbl_sql);
  }

  return info;
}

// Register the created schema internally.
void Logger::logToDbRegisterQueue(SchemaKey key,
                                  std::shared_ptr<AbstractQueue> queue)
{
  this->info(UTL,
             120,
             "Registered schema for {}-{} on table {}.",
             tool_names_[key.tool],
             key.id,
             queue->schema_info().table_name);
  auto registered = schema_registry_.register_schema(key, std::move(queue));

  if (log_db_running_.load(std::memory_order_acquire)) {
    NewSchemaCommand cmd{key, std::move(registered)};
    std::lock_guard<std::mutex> lock(new_schema_queue_mutex_);
    new_schema_queue_.push_back(std::move(cmd));
  }
}

// Promise/futures magic to synchronize caller with backend.
SchemaInfo Logger::syncCreateTable(SchemaKey key,
                                   const char* table_name,
                                   std::string_view header,
                                   const std::vector<SQLiteType>& types)
{
  auto promise = std::promise<SchemaInfo>();
  auto future = promise.get_future();

  {
    std::lock_guard<std::mutex> lock(create_table_mutex_);
    create_table_queue_.push_back(
        {key, table_name, std::string(header), types, std::move(promise)});
  }

  // Block until the backend thread processes this command.
  return future.get();
}


// logDbLoop  (backend thread)
void Logger::logDbLoop()
{
  // Startup: Open the SQLite database and create system tables.
  int rc = sqlite3_open_v2(db_filename_.c_str(),
                           &db_,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                           nullptr);
  // FIXME: Might be be better ways for this thread to throw fatal errors.
  if (rc != SQLITE_OK) {
    db_start_error_ = sqlite3_errmsg(db_)
                          ? sqlite3_errmsg(db_)
                          : "sqlite3_open_v2 returned no message";
    sqlite3_close(db_);
    db_ = nullptr;
    // Signal startup completion (with failure) so startLogDb can throw.
    {
      std::lock_guard<std::mutex> lock(startup_mutex_);
      startup_done_ = true;
    }
    startup_cv_.notify_one();
    log_db_running_ = false;
    return;
  }

  // We do not care about thread safety or atomicity as there is only one backend thread.
  // WAL for max speed.
  sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(db_, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);

  // Create system tables.
  auto exec_sql = [&](const char* sql, const char* desc) -> bool {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
      db_start_error_ = err ? err : desc;
      sqlite3_free(err);
      return false;
    }
    return true;
  };

  if (!exec_sql("CREATE TABLE IF NOT EXISTS tool_names ("
                "tool_id INTEGER PRIMARY KEY, name TEXT)",
                "tool_names")
      || !exec_sql("CREATE TABLE IF NOT EXISTS table_list ("
                   "tool_id INTEGER, message_id INTEGER, table_name TEXT,"
                   " column_types TEXT, column_names TEXT,"
                   " PRIMARY KEY(tool_id, message_id))",
                   "table_list")
      || !exec_sql("CREATE TABLE IF NOT EXISTS metadata ("
                   "tool_id INTEGER, key TEXT, value TEXT)",
                   "metadata")) {
    sqlite3_close(db_);
    db_ = nullptr;
    {
      std::lock_guard<std::mutex> lock(startup_mutex_);
      startup_done_ = true;
    }
    startup_cv_.notify_one();
    log_db_running_ = false;
    return;
  }

  // Populate tool_names table.
  for (int i = 0; i < ToolId::SIZE; ++i) {
    std::string insert
        = fmt::format("INSERT OR REPLACE INTO tool_names VALUES ({}, '{}')",
                      i,
                      tool_names_[i]);
    char* err = nullptr;
    if (sqlite3_exec(db_, insert.c_str(), nullptr, nullptr, &err)
        != SQLITE_OK) {
      db_start_error_ = err ? err : "tool_names insert failed";
      sqlite3_free(err);
      sqlite3_close(db_);
      db_ = nullptr;
      {
        std::lock_guard<std::mutex> lock(startup_mutex_);
        startup_done_ = true;
      }
      startup_cv_.notify_one();
      log_db_running_ = false;
      return;
    }
  }

  // Signal to startLogDb that the backend is ready.
  // IMPORTANT: Any critical faults/checks added in the future MUST be calculated before this.
  // The rest of the code assumes all is well after this point.
  db_ready_ = true;
  {
    std::lock_guard<std::mutex> lock(startup_mutex_);
    startup_done_ = true;
  }
  startup_cv_.notify_one();

  // Main loop spins to drain queues, until stopped flag raised.
  // Uses a fairly simple round robin + mem pressure scheduling mechanism.


  // Local registry for the backend thread: SchemaKey -> AbstractQueue
  std::unordered_map<SchemaKey, std::shared_ptr<AbstractQueue>, SchemaKeyHasher>
      local_registry;

  // Spin until something else lowers the running flag.
  while (log_db_running_.load(std::memory_order_acquire)) {
    bool did_work = false;

    // First, drain all pending CreateTableCommand entries ---
    // These have the highest priority because callers are blocked on them
    // and we cannot do anything else without a registration.
    {
      std::deque<CreateTableCommand> pending_ct;
      {
        std::lock_guard<std::mutex> lock(create_table_mutex_);
        pending_ct.swap(create_table_queue_);
      }
      for (auto& cmd : pending_ct) {
        try {
          SchemaInfo info = logToDbBuildSchemaInfo(
              db_, cmd.key, cmd.table_name, cmd.header, cmd.types);
          cmd.result_promise.set_value(std::move(info));
        } catch (...) {
          // Propagate any exception (e.g. from this->error()) to the
          // caller that is blocked on future.get().
          cmd.result_promise.set_exception(std::current_exception());
        }
        did_work = true;
      }
    }

    // Drain all pending NewSchemaCommand entries
    {
      std::deque<NewSchemaCommand> pending_commands;
      {
        std::lock_guard<std::mutex> lock(new_schema_queue_mutex_);
        pending_commands.swap(new_schema_queue_);
      }

      for (auto& cmd : pending_commands) {
        if (local_registry.find(cmd.key) != local_registry.end()) {
          // Schema re-registration (e.g., after disable/enable):
          // drain any remaining data in the old queue, then replace.
          local_registry[cmd.key]->drain_to_db(db_, SIZE_MAX);
        }
        local_registry[cmd.key] = std::move(cmd.queue);
      }
      did_work = did_work || !pending_commands.empty();
    }

    // // Phase 3: Drain metadata queue
    if (drainMetadataQueue()) {
      did_work = true;
    }

    // --- Phase 4: Schedule and drain queues ---
    size_t total_mem = 0;
    for (auto& entry : local_registry) {
      total_mem += entry.second->approx_size() * entry.second->row_size_bytes();
    }

    bool drained_some = false;

    // --- Global pressure: fully drain the largest queue ---
    bool global_pressure = false;
    if (db_log_global_max_mem_ > 0) {
      const size_t global_limit = static_cast<size_t>(
          db_log_global_max_mem_ * k_queue_mem_high_threshold);
      global_pressure = (total_mem >= global_limit);
    }

    if (global_pressure) {
      AbstractQueue* largest_q = nullptr;
      size_t largest_bytes = 0;
      for (auto& entry : local_registry) {
        const size_t bytes
            = entry.second->approx_size() * entry.second->row_size_bytes();
        if (bytes > largest_bytes) {
          largest_bytes = bytes;
          largest_q = entry.second.get();
        }
      }
      if (largest_q) {
        drained_some |= (largest_q->drain_to_db(db_, SIZE_MAX) > 0);
      }
      did_work |= drained_some;
    } else {
      // --- Per-channel pressure: drain enough to get below 80% ---
      // TODO: Not sure if this is the best way to deal with this problem but crudely it works.
      for (auto& entry : local_registry) {
        if (db_log_per_channel_max_mem_ == 0) {
          continue;
        }

        auto& q = entry.second;
        const size_t channel_bytes = q->approx_size() * q->row_size_bytes();
        const size_t channel_limit = static_cast<size_t>(
            db_log_per_channel_max_mem_ * k_queue_mem_high_threshold);

        if (channel_bytes >= channel_limit) {
          const size_t bytes_to_clear = channel_bytes - channel_limit + 1;
          const size_t row_size = q->row_size_bytes();
          const size_t rows_to_clear
              = (bytes_to_clear + row_size - 1) / row_size;

          drained_some |= (q->drain_to_db(db_, rows_to_clear) > 0);
        }
      }

      // Common case round robin. Not sure how well it will scale.
      // Skip this if under pressure to keep spinning this loop faster.
      if (!drained_some) {
        // --- Round-robin: fully clear every queue ---
        for (auto& entry : local_registry) {
          drained_some |= (entry.second->drain_to_db(db_, SIZE_MAX) > 0);
        }
      }
      did_work |= drained_some;
    }

    if (!did_work) {
      // FIXME: Consider whether to hardcode this or estimate this by some runtime metric.
      // We don't want to waste time, but also we can assume we can have a processor for this.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  // Shutdown helper: keep draining all queues until nothing remains, then close.
  // FIXME: There is no mechanism to block new things from being written to queues.
  // This data will be silently dropped after this finishes. Not a real problem for
  // the vast majority of usecases, but in the future might be worth addressing.
  auto drain_all = [&]() -> bool {
    bool work = false;

    // Process any remaining CreateTable commands.
    {
      std::deque<CreateTableCommand> pending_ct;
      {
        std::lock_guard<std::mutex> lock(create_table_mutex_);
        pending_ct.swap(create_table_queue_);
      }
      for (auto& cmd : pending_ct) {
        try {
          SchemaInfo info = logToDbBuildSchemaInfo(
              db_, cmd.key, cmd.table_name, cmd.header, cmd.types);
          cmd.result_promise.set_value(std::move(info));
        } catch (...) {
          cmd.result_promise.set_exception(std::current_exception());
        }
        work = true;
      }
    }

    // Process any remaining NewSchema commands.
    {
      std::deque<NewSchemaCommand> pending_commands;
      {
        std::lock_guard<std::mutex> lock(new_schema_queue_mutex_);
        pending_commands.swap(new_schema_queue_);
      }
      for (auto& cmd : pending_commands) {
        if (local_registry.find(cmd.key) != local_registry.end()) {
          // Schema re-registration: drain old queue then replace.
          local_registry[cmd.key]->drain_to_db(db_, SIZE_MAX);
        }
        local_registry[cmd.key] = std::move(cmd.queue);
        work = true;
      }
    }
    // Drain all queues entirely.
    work |= drainMetadataQueue();

    for (auto& [key, q] : local_registry) {
      work |= (q->drain_to_db(db_, SIZE_MAX) > 0);
    }

    return work;
  };

  while (drain_all()) {
  }

  // Destroy queues before closing so TypedQueue destructors can finalize
  // their prepared statements.
  local_registry.clear();

  // Checkpoint the WAL into the main database before closing.
  // Without this, data in the WAL may not be fully integrated into the
  // main DB file. TRUNCATE mode checkpoints all pages and resets the WAL
  // file to zero bytes for a clean shutdown.
  sqlite3_exec(db_, "PRAGMA wal_checkpoint(TRUNCATE);", nullptr, nullptr, nullptr);

  sqlite3_close(db_);
  db_ = nullptr;
  db_ready_ = false;
}

ToolId Logger::findToolId(const char* tool_name)
{
  const std::string_view name(tool_name);
  int tool_id = 0;
  for (const char* tool : tool_names_) {
    if (name == tool) {
      return static_cast<ToolId>(tool_id);
    }
    tool_id++;
  }
  return UKN;
}

void Logger::setDebugLevel(ToolId tool, const char* group, int level)
{
  if (level == 0) {
    auto& groups = debug_group_level_[tool];
    auto it = groups.find(group);
    if (it != groups.end()) {
      groups.erase(it);
      debug_on_
          = std::ranges::any_of(debug_group_level_,

                                [](auto& group) { return !group.empty(); });
    }
  } else {
    debug_on_ = true;
    debug_group_level_.at(tool)[group] = level;
  }
}

void Logger::addSink(spdlog::sink_ptr sink)
{
  sinks_.push_back(sink);
  logger_->sinks().emplace_back(std::move(sink));
  setFormatter();  // updates the new sink
}

void Logger::removeSink(const spdlog::sink_ptr& sink)
{
  // remove from local list of sinks_
  auto sinks_find = std::ranges::find(sinks_, sink);
  if (sinks_find != sinks_.end()) {
    sinks_.erase(sinks_find);
  }
  // remove from spdlog list of sinks
  auto& logger_sinks = logger_->sinks();
  auto logger_find = std::ranges::find(logger_sinks, sink);
  if (logger_find != logger_sinks.end()) {
    logger_sinks.erase(logger_find);
  }
}

void Logger::setMetricsStage(std::string_view format)
{
  if (metrics_stages_.empty()) {
    metrics_stages_.emplace(format);
  } else {
    metrics_stages_.top() = format;
  }
}

void Logger::clearMetricsStage()
{
  std::stack<std::string> new_stack;
  metrics_stages_.swap(new_stack);
}

void Logger::pushMetricsStage(std::string_view format)
{
  metrics_stages_.emplace(format);
}

std::string Logger::popMetricsStage()
{
  if (!metrics_stages_.empty()) {
    std::string stage = metrics_stages_.top();
    metrics_stages_.pop();
    return stage;
  }
  return "";
}

void Logger::flushMetrics()
{
  const std::string json = MetricsEntry::assembleJSON(metrics_entries_);

  for (const std::string& sink_path : metrics_sinks_) {
    std::ofstream sink_file(sink_path);
    if (sink_file) {
      sink_file << json;
    } else {
      this->warn(UTL, 10, "Unable to open {} to write metrics", sink_path);
    }
  }
}

void Logger::addWarningMetrics()
{
  // Add metrics for non-zero warnings
  int warning_type_cnt = 0;
  for (int i = 0; i < ToolId::SIZE; ++i) {
    for (int j = 0; j <= max_message_id; ++j) {
      if (message_counters_[i][j] > 0
          && message_levels_[i][j] == spdlog::level::warn) {
        warning_type_cnt++;
        log_metric(
            // NOLINTNEXTLINE(misc-include-cleaner)
            fmt::format("flow__warnings__count:{}-{:04}", tool_names_[i], j),
            std::to_string(message_counters_[i][j]));
      }
    }
  }

  // Add a metric to report the number of unique warning types
  log_metric("flow__warnings__type_count", std::to_string(warning_type_cnt));
}

void Logger::finalizeMetrics()
{
  if (metrics_finalized_) {
    return;
  }
  metrics_finalized_ = true;

  log_metric("flow__warnings__count", std::to_string(warning_count_));
  log_metric("flow__errors__count", std::to_string(error_count_));

  addWarningMetrics();

  for (MetricsPolicy policy : metrics_policies_) {
    policy.applyPolicy(metrics_entries_);
  }

  flushMetrics();
}

void Logger::suppressMessage(ToolId tool, int id)
{
  message_counters_[tool][id] = max_message_print + 1;
}

void Logger::unsuppressMessage(ToolId tool, int id)
{
  message_counters_[tool][id] = 0;
}

void Logger::redirectFileBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_ = std::make_unique<std::ofstream>(filename);
  setRedirectSink(*file_redirect_);
}

void Logger::redirectFileAppendBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_
      = std::make_unique<std::ofstream>(filename, std::ofstream::app);
  setRedirectSink(*file_redirect_);
}

void Logger::redirectFileEnd()
{
  if (file_redirect_ == nullptr) {
    return;
  }

  restoreFromRedirect();

  file_redirect_->close();
  file_redirect_ = nullptr;
}

void Logger::teeFileBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_ = std::make_unique<std::ofstream>(filename);
  setRedirectSink(*file_redirect_, true);
}

void Logger::teeFileAppendBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_
      = std::make_unique<std::ofstream>(filename, std::ofstream::app);
  setRedirectSink(*file_redirect_, true);
}

void Logger::teeFileEnd()
{
  redirectFileEnd();
}

void Logger::redirectStringBegin()
{
  assertNoRedirect();

  string_redirect_ = std::make_unique<std::ostringstream>();
  setRedirectSink(*string_redirect_);
}

std::string Logger::redirectStringEnd()
{
  if (string_redirect_ == nullptr) {
    return "";
  }

  restoreFromRedirect();

  std::string string = string_redirect_->str();
  string_redirect_ = nullptr;

  return string;
}

void Logger::teeStringBegin()
{
  assertNoRedirect();

  string_redirect_ = std::make_unique<std::ostringstream>();
  setRedirectSink(*string_redirect_, true);
}

std::string Logger::teeStringEnd()
{
  return redirectStringEnd();
}

Logger* Logger::defaultLogger()
{
  static Logger default_logger;
  return &default_logger;
}

void Logger::assertNoRedirect()
{
  if (string_redirect_ != nullptr || file_redirect_ != nullptr) {
    this->error(
        UTL, 102, "Unable to start new log redirect while another is active.");
  }
}

void Logger::setRedirectSink(std::ostream& sink_stream, bool keep_sinks)
{
  if (!keep_sinks) {
    logger_->sinks().clear();
  }

  logger_->sinks().push_back(
      std::make_shared<spdlog::sinks::ostream_sink_mt>(sink_stream, true));
  setFormatter();
}

void Logger::restoreFromRedirect()
{
  logger_->sinks().clear();
  logger_->sinks().insert(
      logger_->sinks().begin(), sinks_.begin(), sinks_.end());
}

void Logger::startPrometheusEndpoint(uint16_t port)
{
  if (prometheus_metrics_) {
    return;
  }

  prometheus_metrics_ = std::make_unique<PrometheusMetricsServer>(
      prometheus_registry_, this, port);
}

std::shared_ptr<PrometheusRegistry> Logger::getRegistry()
{
  return prometheus_registry_;
}

bool Logger::isPrometheusServerReadyToServe()
{
  if (!prometheus_metrics_) {
    return false;
  }

  return prometheus_metrics_->is_ready() && prometheus_metrics_->port() != 0;
}

bool Logger::hasPrometheusServerStartupFailed()
{
  if (!prometheus_metrics_) {
    return false;
  }

  return prometheus_metrics_->has_startup_failed();
}

uint16_t Logger::getPrometheusPort()
{
  if (!prometheus_metrics_) {
    return 0;
  }

  return prometheus_metrics_->port();
}

void Logger::setFormatter()
{
  // create formatter without a newline
  std::unique_ptr<spdlog::formatter> formatter
      = std::make_unique<spdlog::pattern_formatter>(
          pattern_, spdlog::pattern_time_type::local, "");
  logger_->set_formatter(std::move(formatter));
}

std::unique_ptr<Progress> Logger::swapProgress(Progress* progress)
{
  std::unique_ptr<Progress> current_progress = std::move(progress_);
  progress_.reset(progress);

  return current_progress;
}

void Logger::setDbLogGlobalMaxMem(size_t bytes)
{
  db_log_global_max_mem_ = bytes;
}

size_t Logger::getDbLogGlobalMaxMem() const
{
  return db_log_global_max_mem_;
}

void Logger::setDbLogPerChannelMaxMem(size_t bytes)
{
  db_log_per_channel_max_mem_ = bytes;
}

size_t Logger::getDbLogPerChannelMaxMem() const
{
  return db_log_per_channel_max_mem_;
}

std::optional<size_t> Logger::logToDbMetadata(ToolId tool,
                                              std::string key,
                                              std::string value)
{
  if (!db_ready_) {
    return std::nullopt;
  }
  std::lock_guard<std::mutex> lock(metadata_queue_mutex_);
  metadata_queue_.emplace(tool, std::move(key), std::move(value));
  return 1;
}

bool Logger::drainMetadataQueue()
{
  std::queue<MetadataRow> local_meta;
  {
    std::lock_guard<std::mutex> lock(metadata_queue_mutex_);
    local_meta.swap(metadata_queue_);
  }

  if (local_meta.empty()) {
    return false;
  }

  sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);
  bool meta_ok = true;
  while (!local_meta.empty()) {
    auto [mtool, mkey, mval] = std::move(local_meta.front());
    local_meta.pop();

    char* sql = sqlite3_mprintf("INSERT INTO metadata VALUES (%d, %Q, %Q)",
                                static_cast<int>(mtool),
                                mkey.c_str(),
                                mval.c_str());
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
      this->warn(UTL,
                 116,
                 "Failed to insert metadata row for tool={} key='{}': {}",
                 static_cast<int>(mtool),
                 mkey,
                 sqlite3_errmsg(db_));
      meta_ok = false;
      break;
    }
  }
  sqlite3_exec(db_, meta_ok ? "COMMIT" : "ROLLBACK", nullptr, nullptr, nullptr);
  return true;
}

bool Logger::isDbLogEnabled(SchemaKey key) const
{
  std::lock_guard<std::mutex> lock(db_log_enabled_mutex_);
  return db_log_disabled_set_.find(key) == db_log_disabled_set_.end();
}

void Logger::setDbLogEnabled(ToolId tool, int id, bool enabled)
{
  this->info(UTL, 119, "Database logging for {}-{} is {}.",
             tool_names_[tool], id, enabled ? "enabled" : "disabled");
  SchemaKey key{tool, id};
  std::lock_guard<std::mutex> lock(db_log_enabled_mutex_);
  if (enabled) {
    db_log_disabled_set_.erase(key);
  } else {
    db_log_disabled_set_.insert(key);
    // If already registered, remove it so the next logToDb call
    // hits the slow path and re-checks isDbLogEnabled.
    schema_registry_.remove_schema(key);
  }
}

bool Logger::getDbLogEnabled(ToolId tool, int id) const
{
  return isDbLogEnabled({tool, id});
}

}  // namespace utl
