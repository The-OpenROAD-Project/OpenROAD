#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

#include "utl/Logger.h"

namespace utl {

inline bool envVarTruthy(const char* name)
{
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return false;
  }

  std::string value(raw);
  const size_t start = value.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return false;
  }
  const size_t end = value.find_last_not_of(" \t\n\r");
  value = value.substr(start, end - start + 1);
  std::ranges::transform(value, value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value == "1" || value == "true" || value == "yes" || value == "on";
}

inline std::optional<float> getEnvFloat(const char* name,
                                        Logger* logger,
                                        ToolId tool,
                                        int message_id)
{
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return std::nullopt;
  }

  char* end = nullptr;
  const float value = std::strtof(raw, &end);
  if (end == raw || (end != nullptr && *end != '\0')) {
    if (logger != nullptr) {
      logger->warn(
          tool, message_id, "Ignoring {}='{}' (not a valid float).", name, raw);
    }
    return std::nullopt;
  }
  return value;
}

inline std::optional<float> getEnvFloat(const char* name)
{
  return getEnvFloat(name, nullptr, UTL, 0);
}

}  // namespace utl
