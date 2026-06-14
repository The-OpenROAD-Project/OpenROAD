// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <list>
#include <map>
#include <memory>
#include <atomic>
#include <ostream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef SWIG
// Everything behind this guard is invisible to SWIG because the
// parser cannot handle:
//   - <sqlite3.h> (C callbacks, opaque types)
//   - mutex-protected queues (templates, C++ atomics)
//   - Template-heavy helpers (std::tuple, fold expressions,
//     index_sequence, TypedQueue<Args...>)
// The corresponding .cpp code is still compiled normally.
#include <chrono>
#include <sqlite3.h>
#include <tuple>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <deque>
#include <future>
#include <condition_variable>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#endif

#include "spdlog/common.h"
#include "spdlog/details/os.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/logger.h"
#include "utl/Metrics.h"
#if FMT_VERSION >= 110000
#include "spdlog/fmt/ranges.h"
#endif

#include "spdlog/spdlog.h"

namespace utl {

class PrometheusMetricsServer;
class PrometheusRegistry;

class Progress;

// Keep this sorted
#define FOREACH_TOOL(X) \
  X(ANT)                \
  X(CGT)                \
  X(CHK)                \
  X(CTS)                \
  X(CUT)                \
  X(DFT)                \
  X(DPL)                \
  X(DRT)                \
  X(DST)                \
  X(EST)                \
  X(EXA)                \
  X(FIN)                \
  X(FLW)                \
  X(GPL)                \
  X(GRT)                \
  X(GUI)                \
  X(IFP)                \
  X(MPL)                \
  X(ODB)                \
  X(ORD)                \
  X(PAD)                \
  X(PAR)                \
  X(PDN)                \
  X(PPL)                \
  X(PSM)                \
  X(RAM)                \
  X(RCX)                \
  X(RMP)                \
  X(RSZ)                \
  X(STA)                \
  X(STT)                \
  X(SYN)                \
  X(TAP)                \
  X(TST)                \
  X(UKN)                \
  X(UPF)                \
  X(UTL)                \
  X(WEB)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

// backward compatibility with fmt versions older than 8
#if FMT_VERSION >= 80000
#define FMT_RUNTIME(format_string) fmt::runtime(format_string)
#else
#define FMT_RUNTIME(format_string) format_string
#endif

enum ToolId
{
  FOREACH_TOOL(GENERATE_ENUM)
      SIZE  // the number of tools, do not put anything after this
};

#ifndef SWIG
// --- SQLite Logging Types ---

// Numerics Only
enum class SQLiteType {
  INTEGER,
  REAL
};

// tool/message pair
struct SchemaKey {
  ToolId tool;
  int id;

  bool operator==(const SchemaKey& other) const {
    return tool == other.tool && id == other.id;
  }
};

// Helper for hashmapping the tool/message pair
struct SchemaKeyHasher {
  size_t operator()(const SchemaKey& k) const {
    return std::hash<int>{}(static_cast<int>(k.tool)) ^ (std::hash<int>{}(k.id) << 1);
  }
};

struct ColumnDefinition {
  std::string name;
  SQLiteType type;
};

// For the binary dump tables
struct SchemaInfo {
  std::vector<ColumnDefinition> columns;
  std::string table_name;
};

// Type-to-SQLiteType mapping trait. Template magic to detect int vs real.
template <typename T, typename Enabler = void>
struct TypeToSQLite;

template <typename T>
struct TypeToSQLite<T, std::enable_if_t<std::is_integral_v<T>>> {
  static constexpr SQLiteType value = SQLiteType::INTEGER;
};

template <typename T>
struct TypeToSQLite<T, std::enable_if_t<std::is_floating_point_v<T>>> {
  static constexpr SQLiteType value = SQLiteType::REAL;
};

// Type-erased version of the templated concrete queue.
// This type erasure magic is what allows the whole thing to work reasonably sanely,
// because the caller can use templates for queueing to avoid high serialization costs,
// while the worker thread can later make decisions independently of the exact type used.
class AbstractQueue {
public:
  AbstractQueue() = default;
  AbstractQueue(const AbstractQueue&) = delete;
  AbstractQueue& operator=(const AbstractQueue&) = delete;
  virtual ~AbstractQueue() = default;

  const SchemaInfo& schema_info() const { return info_; }
  virtual size_t row_size_bytes() const = 0;

  size_t approx_size() const {
    return item_count_.load(std::memory_order_acquire);
  }

  uint64_t last_flush_timestamp_ms() const {
    return last_flush_ms_.load(std::memory_order_acquire);
  }

  virtual size_t drain_to_db(sqlite3* db, size_t max_records) = 0;

protected:
  SchemaInfo info_;
  std::atomic<size_t> item_count_{0};
  std::atomic<uint64_t> last_flush_ms_{0};
};

template <typename... Args>
class TypedQueue : public AbstractQueue {
public:
  using row_type = std::tuple<Args...>;

  // We don't want accidental conversion to the AbstractQueue.
  explicit TypedQueue(SchemaInfo info) { info_ = std::move(info); }
  ~TypedQueue() override {
    if (stmt_) {
      sqlite3_finalize(stmt_);
    }
  }

  // Back of the napkin estimate at compile time but should be good enough for numerics
  size_t row_size_bytes() const override { return sizeof(row_type); }

  bool push(row_type row, const std::atomic<bool>& /*abort_flag*/)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(row));
    item_count_.fetch_add(1, std::memory_order_release);
    return true;
  }

  // Acquires queue mutex, and drains n records to the database. Most optimal to just drain the whole thing unless there are other reasons to not do so.
  size_t drain_to_db(sqlite3* db, size_t max_records) override
  {
    // The latter means the statement could not be built, so fail the whole thing. Otherwise use cached stmt.
    if (!stmt_ && !build_insert_stmt(db)) {
      return 0;
    }

    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);

    size_t count = 0;
    bool ok = true;

    while (count < max_records) {
      row_type row;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
          break;
        }
        row = std::move(queue_.front());
        queue_.pop_front();
        item_count_.fetch_sub(1, std::memory_order_release);
      }

      // C++17 template magic to pass the column name strings as column keys for sqlite
      bind_row(stmt_, row, std::index_sequence_for<Args...>{});
      int rc = sqlite3_step(stmt_);
      sqlite3_reset(stmt_);
      if (rc != SQLITE_DONE) {
        ok = false;
        break;
      }
      ++count;
    }

    // Rollback if there was a failure.
    if (count > 0) {
      sqlite3_exec(db, ok ? "COMMIT" : "ROLLBACK",
                   nullptr, nullptr, nullptr);
    }

    last_flush_ms_.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count(),
        std::memory_order_release);
    return count;
  }

private:
  // More magic to dynamically bind int/reals
  template <typename T>
  static void bind_field(sqlite3_stmt* stmt, int idx, T value)
  {
    if constexpr (TypeToSQLite<T>::value == SQLiteType::INTEGER) {
      sqlite3_bind_int64(stmt, idx, static_cast<sqlite3_int64>(value));
    } else if constexpr (TypeToSQLite<T>::value == SQLiteType::REAL) {
      sqlite3_bind_double(stmt, idx, static_cast<double>(value));
    }
  }

  template <size_t... Is>
  void bind_row(sqlite3_stmt* stmt,
                const row_type& row,
                std::index_sequence<Is...>) const
  {
    ((bind_field(stmt, static_cast<int>(Is) + 1, std::get<Is>(row))), ...);
  }

  // FIXME: I think we can do better than string appending with loops but this will do for now.
  bool build_insert_stmt(sqlite3* db)
  {
    std::string sql = "INSERT INTO " + info_.table_name + " (";
    for (size_t i = 0; i < info_.columns.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += info_.columns[i].name;
    }
    sql += ") VALUES (";
    for (size_t i = 0; i < info_.columns.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += "?";
    }
    sql += ");";
    return sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) == SQLITE_OK;
  }

  std::deque<row_type> queue_;
  mutable std::mutex mutex_;
  sqlite3_stmt* stmt_ = nullptr;
};

// Schema registration infrastructure below. Maybe worth it to merge it with the queue system if performance is not a problem.
// Command sent from caller thread to the backend thread
// when a new schema is discovered.
struct NewSchemaCommand {
  SchemaKey key;
  std::shared_ptr<AbstractQueue> queue;
};

// Command sent from caller thread to the backend thread to create a SQLite
// table and register its schema.  The caller blocks on result_promise until
// the backend has executed it.
struct CreateTableCommand {
  SchemaKey key;
  std::string table_name;
  std::string header;
  std::vector<SQLiteType> types;
  std::promise<SchemaInfo> result_promise;
};

// unordered map with some helpers
class SchemaRegistry {
public:
  using RegistryMap
      = std::unordered_map<SchemaKey, std::shared_ptr<AbstractQueue>, SchemaKeyHasher>;

  SchemaRegistry() : registry_ptr_(std::make_shared<const RegistryMap>()) {}

  std::shared_ptr<const RegistryMap> get_map() const {
    return std::atomic_load(&registry_ptr_);
  }

  std::shared_ptr<AbstractQueue> register_schema(
      SchemaKey key,
      std::shared_ptr<AbstractQueue> queue)
  {
    std::lock_guard<std::mutex> lock(registration_mutex_);
    auto current_map = get_map();
    auto it = current_map->find(key);
    if (it != current_map->end()) {
      return it->second;
    }
    auto new_map = std::make_shared<RegistryMap>(*current_map);
    (*new_map)[key] = queue;
    std::atomic_store(
        &registry_ptr_,
        std::const_pointer_cast<const RegistryMap>(new_map));
    return queue;
  }

  void remove_schema(SchemaKey key) {
    std::lock_guard<std::mutex> lock(registration_mutex_);
    auto current_map = get_map();
    auto it = current_map->find(key);
    if (it == current_map->end()) {
      return;
    }
    auto new_map = std::make_shared<RegistryMap>(*current_map);
    new_map->erase(key);
    std::atomic_store(
        &registry_ptr_,
        std::const_pointer_cast<const RegistryMap>(new_map));
  }

private:
  std::shared_ptr<const RegistryMap> registry_ptr_;
  std::mutex registration_mutex_;
};

// Template magic to enforce valid column names for the database at compile time.
template <size_t N>
struct FixedString {
  char data[N]{};
  constexpr FixedString() = default;
  constexpr FixedString(const char (&str)[N]) {
    for (size_t i = 0; i < N; ++i) data[i] = str[i];
  }

  constexpr size_t count_fields() const {
    if (data[0] == '\0') return 0;
    size_t count = 1;
    for (size_t i = 0; i < N; ++i) {
      if (data[i] == '\0') break;
      if (data[i] == ',') {
        size_t j = i + 1;
        while (j < N && data[j] == ' ') j++;
        if (j < N && data[j] != '\0' && data[j] != ',') count++;
      }
    }
    return count;
  }

  constexpr bool isValid() const {
    bool empty = true;
    for (size_t i = 0; i < N; ++i) {
      char c = data[i];
      if (c == '\0') break;
      if (c == ',') {
        if (empty) return false;
        empty = true;
      } else if (c == '_' || c == ' ' || (c >= 'a' && c <= 'z')
                 || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        empty = false;
      } else {
        // Reject any character that is not alphanumeric, underscore,
        // comma, or space — prevents SQL injection through column names.
        return false;
      }
    }
    return !empty;
  }

  constexpr size_t field_start(size_t idx) const {
    size_t cur = 0;
    for (size_t i = 0; i < N; ++i) {
      if (cur == idx) {
        while (i < N && data[i] == ' ') i++;
        return i;
      }
      if (data[i] == ',') cur++;
      if (data[i] == '\0') break;
    }
    return N;
  }

  constexpr size_t field_end(size_t idx) const {
    size_t start = field_start(idx);
    if (start >= N) return N;
    size_t last = start;
    for (size_t i = start; i < N && data[i] != ',' && data[i] != '\0'; ++i) {
      if (data[i] != ' ') last = i + 1;
    }
    return last;
  }
};

// Value type extracted from an iterator — used by logToDbBulk.
template <typename Iter>
using iter_val_t = typename std::iterator_traits<Iter>::value_type;

#endif // End the SWIG guard

class Logger
{
 public:
  // Use nullptr if messages or metrics are not logged to a file.
  Logger(const char* filename = nullptr,
         const char* metrics_filename = nullptr);
  Logger(const Logger& logger) = delete;
  ~Logger();
  static ToolId findToolId(const char* tool_name);

  template <typename... Args>
  void report(const std::string& message, const Args&... args)
  {
    logger_->log(spdlog::level::level_enum::off,
                 FMT_RUNTIME(message + spdlog::details::os::default_eol),
                 args...);
  }

  // Reports a string literal with no interpolation or newline.
  template <typename... Args>
  void reportLiteral(const std::string& message)
  {
    logger_->log(spdlog::level::level_enum::off, "{}", message);
  }

  // Do NOT call this directly, use the debugPrint macro  instead (defined
  // below)
  template <typename... Args>
  void debug(ToolId tool,
             const char* group,
             const std::string& message,
             const Args&... args)
  {
    // Message counters do NOT apply to debug messages.
    logger_->log(
        spdlog::level::level_enum::debug,
        FMT_RUNTIME("[{} {}-{}] " + message + spdlog::details::os::default_eol),
        level_names[spdlog::level::level_enum::debug],
        tool_names_[tool],
        group,
        args...);
    logger_->flush();
  }

  template <typename... Args>
  void info(ToolId tool,
            int id,
            const std::string& message,
            const Args&... args)
  {
    log(tool, spdlog::level::level_enum::info, id, message, args...);
  }

  template <typename... Args>
  void warn(ToolId tool,
            int id,
            const std::string& message,
            const Args&... args)
  {
    warning_count_++;
    log(tool, spdlog::level::level_enum::warn, id, message, args...);
  }

  template <typename... Args>
  __attribute__((noreturn)) void error(ToolId tool,
                                       int id,
                                       const std::string& message,
                                       const Args&... args)
  {
    error_count_++;
    log(tool, spdlog::level::err, id, message, args...);
    // Exception should be caught by swig error handler.
    throw std::runtime_error(fmt::format("{}-{:04}", tool_names_[tool], id));
  }

  template <typename... Args>
  __attribute__((noreturn)) void critical(ToolId tool,
                                          int id,
                                          const std::string& message,
                                          const Args&... args)
  {
    log(tool, spdlog::level::level_enum::critical, id, message, args...);
    exit(EXIT_FAILURE);
  }

  // For logging to the metrics file.  This is a much more restricted
  // API as we are writing JSON not user messages.
  // Note: these methods do no escaping so avoid special characters.
  template <typename T, typename U = std::enable_if_t<std::is_arithmetic_v<T>>>
  void metric(const std::string_view metric_name, T value)
  {
    const std::string name = std::string(metric_name);
    if (std::isinf(value)) {
      if (value < 0) {
        metric(name, "-Infinity");
      } else {
        metric(name, "Infinity");
      }
    } else if (std::isnan(value)) {
      metric(name, "NaN");
    } else {
      std::ostringstream oss;
      oss << std::defaultfloat << std::setprecision(6) << value;
      log_metric(name, oss.str());
    }
  }

  void metric(const std::string_view metric, const std::string& value)
  {
    log_metric(std::string(metric), '"' + value + '"');
  }

  void setDebugLevel(ToolId tool, const char* group, int level);

  bool debugCheck(ToolId tool, const char* group, int level) const
  {
    if (!debug_on_) {
      return false;
    }
    auto& groups = debug_group_level_[tool];
    auto it = groups.find(group);
    return (it != groups.end() && level <= it->second);
  }

  int getWarningCount() const { return warning_count_; }

  void startPrometheusEndpoint(uint16_t port);
  std::shared_ptr<PrometheusRegistry> getRegistry();
  bool isPrometheusServerReadyToServe();
  bool hasPrometheusServerStartupFailed();
  uint16_t getPrometheusPort();

  void suppressMessage(ToolId tool, int id);
  void unsuppressMessage(ToolId tool, int id);

#ifndef SWIG
  // Logs structured data to the SQLite database.
  // The schema (types of Args...) is registered lazily on the first call
  // for a (tool, id) pair.  The template body is intentionally thin:
  // compile-time checks live here, all runtime logic is in the .cpp.
  template <FixedString Header, typename... RawArgs>
  std::optional<size_t> logToDb(ToolId tool, int id, const char* tableName, RawArgs&&... raw_args)
  {
    // If database logging is not running, silently skip.
    if (!db_ready_) {
      return std::nullopt;
    }

    static_assert(
        Header.isValid(),
        "Header must be a comma-separated list of alphanumeric names"
        " and underscores.");
    static_assert(
        sizeof...(RawArgs) == Header.count_fields(),
        "Number of arguments provided to logToDb must match the number"
        " of fields in the header.");

    // Build a compile-time array of SQLiteTypes for each argument.
    constexpr std::array<SQLiteType, sizeof...(RawArgs)> types_arr
        = {TypeToSQLite<std::decay_t<RawArgs>>::value...};

    SchemaKey key{tool, id};

    // If this (tool, id) pair has been explicitly disabled, skip silently.
    if (!isDbLogEnabled(key)) {
      return std::nullopt;
    }

    auto queue_opt = logToDbFindQueue(key);
    if (!queue_opt.has_value()) {
      // send CreateTableCommand to backend, create TypedQueue, register it---
      SchemaInfo info = syncCreateTable(
          key,
          tableName,
          std::string_view(Header.data),
          std::vector<SQLiteType>(types_arr.begin(), types_arr.end()));
      auto queue = std::make_shared<TypedQueue<std::decay_t<RawArgs>...>>(std::move(info));
      logToDbRegisterQueue(key, std::move(queue));
      // Re-fetch after registration so the data is pushed.
      queue_opt = logToDbFindQueue(key);
      if (!queue_opt.has_value()) {
        return std::nullopt;
      }
    }

    // --- FAST PATH (also reached from SLOW PATH after re-fetch): ---
    auto& base = queue_opt.value();
    auto* typed = static_cast<TypedQueue<std::decay_t<RawArgs>...>*>(base.get());
    typed->push(
        // Construct a tuple of value types (strip references) from the
        // forwarded arguments so the queue owns the data.
        std::tuple<std::decay_t<RawArgs>...>(std::forward<RawArgs>(raw_args)...),
        log_db_running_);
    return 1;
  }

  // Bulk version of logToDb: takes one iterator per column and pushes
  // 'count' rows at once.
  template <FixedString Header, typename... InputIters>
  std::optional<size_t> logToDbBulk(ToolId tool, int id, const char* tableName, size_t count, InputIters... iters)
  {
    // If database logging is not running, silently skip.
    if (!db_ready_) {
      return std::nullopt;
    }

    static_assert(
        Header.isValid(),
        "Header must be a comma-separated list of alphanumeric names"
        " and underscores.");
    static_assert(
        sizeof...(InputIters) == Header.count_fields(),
        "Number of iterators provided to logToDbBulk must match the"
        " number of fields in the header.");

    // Check every iterator's value type at compile time.
    constexpr bool all_numeric
        = (... && std::is_arithmetic_v<iter_val_t<InputIters>>);
    static_assert(
        all_numeric,
        "All iterator value types must be arithmetic"
        " (maps to INTEGER or REAL in SQLite).");

    constexpr std::array<SQLiteType, sizeof...(InputIters)> types_arr
        = {TypeToSQLite<iter_val_t<InputIters>>::value...};

    SchemaKey key{tool, id};
    if (!isDbLogEnabled(key)) {
      return std::nullopt;
    }

    auto queue_opt = logToDbFindQueue(key);
    if (!queue_opt.has_value()) {
      SchemaInfo info = syncCreateTable(
          key,
          tableName,
          std::string_view(Header.data),
          std::vector<SQLiteType>(types_arr.begin(), types_arr.end()));
      auto queue = std::make_shared<TypedQueue<iter_val_t<InputIters>...>>(
          std::move(info));
      logToDbRegisterQueue(key, std::move(queue));
      // Re-fetch after registration so the batch can be pushed.
      queue_opt = logToDbFindQueue(key);
      if (!queue_opt.has_value()) {
        return std::nullopt;
      }
    }

    auto* typed
        = static_cast<TypedQueue<iter_val_t<InputIters>...>*>(queue_opt->get());
    for (size_t i = 0; i < count; ++i) {
      typed->push(
          std::make_tuple(std::move(*iters++)...), log_db_running_);
    }
    return count;
  }

  // Enqueue a metadata row (tool, key, value) for the backend to write
  // to the 'metadata' table (both key and value are TEXT).
  std::optional<size_t> logToDbMetadata(ToolId tool, std::string key, std::string value);
#endif

  void addSink(spdlog::sink_ptr sink);
  void removeSink(const spdlog::sink_ptr& sink);
  void addMetricsSink(const char* metrics_filename);
  void removeMetricsSink(const char* metrics_filename);
  void startLogDb(const char* filename);
  void stopLogDb();

  // Maximum total memory (in bytes) the user is willing to let all buffered
  // log-db queues consume before the backend applies backpressure or drops
  // data.  0 means unlimited.
  void setDbLogGlobalMaxMem(size_t bytes);
  size_t getDbLogGlobalMaxMem() const;

  // Maximum memory (in bytes) a single per-schema queue may consume before
  // the backend applies backpressure or drops data on that channel.
  // 0 means unlimited.
  void setDbLogPerChannelMaxMem(size_t bytes);
  size_t getDbLogPerChannelMaxMem() const;

  // Enable or disable database logging for a specific (tool, id) pair.
  // Disabled entries are silently skipped in logToDb.  If a schema was
  // already registered, it is removed so the next logToDb call triggers
  // a fresh registration check.
  void setDbLogEnabled(ToolId tool, int id, bool enabled);
  // Returns true if logging is enabled for (tool, id).  Default is true
  // for all pairs not explicitly disabled.
  bool getDbLogEnabled(ToolId tool, int id) const;

  void setMetricsStage(std::string_view format);
  void clearMetricsStage();
  void pushMetricsStage(std::string_view format);
  std::string popMetricsStage();

  // interface from sta::Report
  // Redirect output to filename until redirectFileEnd is called.
  void redirectFileBegin(const std::string& filename);
  // Redirect append output to filename until redirectFileEnd is called.
  void redirectFileAppendBegin(const std::string& filename);
  void redirectFileEnd();
  // Redirect output to a string until redirectStringEnd is called.
  void redirectStringBegin();
  std::string redirectStringEnd();
  // Tee output to filename until teeFileEnd is called.
  void teeFileBegin(const std::string& filename);
  // Tee append output to filename until teeFileEnd is called.
  void teeFileAppendBegin(const std::string& filename);
  void teeFileEnd();
  // Redirect output to a string until teeStringEnd is called.
  void teeStringBegin();
  std::string teeStringEnd();

  static Logger* defaultLogger();

  // Progress interface
  Progress* progress() const { return progress_.get(); }
  std::unique_ptr<Progress> swapProgress(Progress* progress);

  void finalizeMetrics();

 private:
  std::vector<std::string> metrics_sinks_;
  std::list<MetricsEntry> metrics_entries_;
  std::vector<MetricsPolicy> metrics_policies_;

  std::unique_ptr<Progress> progress_;

  template <typename... Args>
  void log(ToolId tool,
           spdlog::level::level_enum level,
           int id,
           const std::string& message,
           const Args&... args)
  {
    assert(id >= 0 && id <= max_message_id);
    message_levels_[tool][id].store(level, std::memory_order_relaxed);
    auto& counter = message_counters_[tool][id];
    auto count = counter++;
    if (count < max_message_print) {
      logger_->log(level,
                   FMT_RUNTIME("[{} {}-{:04d}] " + message
                               + spdlog::details::os::default_eol),
                   level_names[level],
                   tool_names_[tool],
                   id,
                   args...);
      return;
    }

    if (count == max_message_print) {
      logger_->log(level,
                   "[{} {}-{:04d}] message limit ({})"
                   " reached. This message will no longer print.{}",
                   level_names[level],
                   tool_names_[tool],
                   id,
                   max_message_print,
                   spdlog::details::os::default_eol);
    } else {
      counter--;  // to avoid counter overflow
    }
  }

  void log_metric(const std::string& metric, const std::string& value)
  {
    std::string key;
    if (metrics_stages_.empty()) {
      key = metric;
    } else {
      key = fmt::format(FMT_RUNTIME(metrics_stages_.top()), metric);
    }
    metrics_entries_.push_back({std::move(key), value});
  }

  void flushMetrics();
  // Add new metrics for non-zero warnings. It also counts the number of
  // unique warning types.
  void addWarningMetrics();

  void setRedirectSink(std::ostream& sink_stream, bool keep_sinks = false);
  void restoreFromRedirect();
  void assertNoRedirect();

  void setFormatter();

  // Allows for lookup by a compatible key (ie string_view)
  // to avoid constructing a key (string) just for lookup
  struct StringViewCmp
  {
    using is_transparent = std::true_type;  // enabler
    bool operator()(const std::string_view a, const std::string_view b) const
    {
      return a < b;
    }
  };
  using DebugGroups = std::map<std::string, int, StringViewCmp>;

  static constexpr int max_message_id = 9999;

  // Stop issuing messages of a given tool/id when this limit is hit.
  static constexpr int max_message_print = 1000;

  // Backend scheduler: when a queue reaches this fraction of its memory
  // limit, it is considered under pressure and gets drained aggressively.
  // TODO: Consider what to hardcode this to.
  static constexpr double k_queue_mem_high_threshold = 0.8;

  std::vector<spdlog::sink_ptr> sinks_;
  std::shared_ptr<spdlog::logger> logger_;
  std::stack<std::string> metrics_stages_;

  // interface to handle string and file redirections
  std::unique_ptr<std::ostringstream> string_redirect_;
  std::unique_ptr<std::ofstream> file_redirect_;

  // Prometheus server metrics collection
  std::shared_ptr<PrometheusRegistry> prometheus_registry_;
  std::unique_ptr<PrometheusMetricsServer> prometheus_metrics_;

#ifndef SWIG
  // The SQLite handle is accessed ONLY by the backend (logDbLoop) thread.
  sqlite3* db_ = nullptr;
  std::thread log_db_thread_;
  std::atomic<bool> log_db_running_{false};

  // Main thread checks this atomic instead of reading db_ directly
  // (which is owned by the backend thread after startup).
  std::atomic<bool> db_ready_{false};
  std::string db_filename_;

  // Maximum global memory footprint from buffered log-db data (bytes).
  // 0 = unlimited.  Used by the backend to decide when to throttle.
  size_t db_log_global_max_mem_ = 0;
  // Maximum per-channel (per-registration) memory from buffered data (bytes).
  // 0 = unlimited.
  size_t db_log_per_channel_max_mem_ = 0;

  SchemaRegistry schema_registry_;

  // Command queue: caller threads enqueue NewSchemaCommand,
  // the backend thread (logDbLoop) drains and processes them.
  std::deque<NewSchemaCommand> new_schema_queue_;
  std::mutex new_schema_queue_mutex_;

  // Command queue for creating SQLite tables.  The caller blocks on
  // the embedded promise until the backend completes registration.
  std::deque<CreateTableCommand> create_table_queue_;
  std::mutex create_table_mutex_;

  // Metadata queue: low-traffic text-to-text rows written to the
  // 'metadata' table.  Uses a std::queuen because
  // std::string has non-trivial copy semantics.
  using MetadataRow = std::tuple<ToolId, std::string, std::string>;
  std::queue<MetadataRow> metadata_queue_;
  std::mutex metadata_queue_mutex_;

  // Startup synchronization: startLogDb blocks until the backend thread
  // has opened the SQLite database and created system tables.
  std::mutex startup_mutex_;
  std::condition_variable startup_cv_;
  bool startup_done_ = false;
  std::string db_start_error_;

  // Per-schema enable/disable set.  Logging is enabled by default for
  // all (tool, id) pairs.  Insert a key here to disable it.
  std::unordered_set<SchemaKey, SchemaKeyHasher> db_log_disabled_set_;
  mutable std::shared_mutex db_log_enabled_mutex_;

  // Returns false if the key is in db_log_disabled_set_.
  bool isDbLogEnabled(SchemaKey key) const;

  // Non-template helpers called by the thin logToDb template.
  // Implemented in Logger.cpp.
  std::optional<std::shared_ptr<AbstractQueue>> logToDbFindQueue(SchemaKey key);
  SchemaInfo logToDbBuildSchemaInfo(sqlite3* db,
                                    SchemaKey key,
                                    const std::string& table_name,
                                    std::string_view header,
                                    const std::vector<SQLiteType>& types);
  void logToDbRegisterQueue(SchemaKey key,
                             std::shared_ptr<AbstractQueue> queue);

  // Sends a CreateTableCommand to the backend thread and blocks until
  // the table has been created and the SchemaInfo is returned.  Called
  // from logToDb / logToDbBulk slow paths.
  SchemaInfo syncCreateTable(SchemaKey key,
                              const char* table_name,
                              std::string_view header,
                              const std::vector<SQLiteType>& types);

  // --- Database log worker thread (entry point + phases) ---

  // Entry to database log worker thread.  Launched from startLogDb.
  void logDbLoop();

  // Phase 1: Open the SQLite database, create system tables, populate
  // tool_names, and signal readiness.  Returns true on success.
  bool logDbStartup();

  // Phase 2: Main spin loop.  Drains command queues, schedules work
  // across registered data channels, and applies memory-pressure
  // gating.  Runs until log_db_running_ becomes false.
  void logDbMainLoop(
      std::unordered_map<SchemaKey, std::shared_ptr<AbstractQueue>,
                         SchemaKeyHasher>& local_registry);

  // Phase 3: Final drain of all remaining data, WAL checkpoint, and
  // close of the SQLite database.  local_registry is cleared before
  // close so TypedQueue destructors can finalize prepared statements.
  void logDbShutdown(
      std::unordered_map<SchemaKey, std::shared_ptr<AbstractQueue>,
                         SchemaKeyHasher>& local_registry);

  // Drain all pending metadata rows into the 'metadata' table.
  // Returns true if any rows were drained.
  // Caller must ensure db_ is valid.  Called from logDbLoop and stopLogDb.
  bool drainMetadataQueue();
#endif

  // This matrix is pre-allocated so it can be safely updated
  // from multiple threads without locks.
  using MessageCounter = std::array<std::atomic_int16_t, max_message_id + 1>;
  std::array<MessageCounter, ToolId::SIZE> message_counters_;
  using MessageLevel
      = std::array<std::atomic<spdlog::level::level_enum>, max_message_id + 1>;
  std::array<MessageLevel, ToolId::SIZE> message_levels_;
  std::array<DebugGroups, ToolId::SIZE> debug_group_level_;
  bool debug_on_{false};
  bool metrics_finalized_{false};
  std::atomic_int warning_count_{0};
  std::atomic_int error_count_{0};
  static constexpr const char* level_names[]
      = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "OFF"};
  static constexpr const char* pattern_ = "%v";
  static constexpr const char* tool_names_[] = {FOREACH_TOOL(GENERATE_STRING)};
};

// Use this macro for any debug messages.  It avoids evaluating message and
// varargs when no message is issued.
#define debugPrint(logger, tool, group, level, ...) \
  if (logger->debugCheck(tool, group, level)) {     \
    logger->debug(tool, group, ##__VA_ARGS__);      \
  }

#undef FOREACH_TOOL
#undef GENERATE_ENUM
#undef GENERATE_STRING

}  // namespace utl

// Define stream_as for fmt > v10
#if !SWIG && FMT_VERSION >= 100000

namespace utl {

struct test_ostream
{
 public:
  template <class T>
  static auto test(int) -> decltype(std::declval<std::ostream>()
                                        << std::declval<T>(),
                                    std::true_type());

  template <class>
  static auto test(...) -> std::false_type;
};

template <class T,
          class = std::enable_if_t<decltype(test_ostream::test<T>(0))::value>>
auto format_as(const T& t)
{
  return fmt::streamed(t);
}

}  // namespace utl

#else
namespace utl {

// Uncallable class
template <class T>
class format_as
{
};

}  // namespace utl
#endif
